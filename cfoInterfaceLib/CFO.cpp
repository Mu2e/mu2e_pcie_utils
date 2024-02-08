
#include "TRACE/tracemf.h"
#define TRACE_NAME "CFO.cpp"

#include "CFO.h"
#define TLVL_GetData TLVL_DEBUG + 5
#define TLVL_ReadBuffer TLVL_DEBUG + 7
#define TLVL_ReadNextDAQPacket TLVL_DEBUG + 8
#define TLVL_ReleaseBuffers TLVL_DEBUG + 20
#define TLVL_GetCurrentBuffer TLVL_DEBUG + 21

#include "dtcInterfaceLib/otsStyleCoutMacros.h"

#define CFO_TLOG(lvl) TLOG(lvl) << "CFO " << device_.getDeviceUID() << ": "
#undef __COUT_HDR__
#define __COUT_HDR__  "CFO " <<  device_.getDeviceUID() << ": "

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>  // Convert uint to hex string

CFOLib::CFO::CFO(DTC_SimMode mode, int cfo, std::string expectedDesignVersion, bool skipInit, const std::string& uid)
	: CFO_Registers(mode, cfo, expectedDesignVersion, skipInit, uid), daqDMAInfo_(), dcsDMAInfo_()
{
	__COUT_INFO__ << "CONSTRUCTOR";
}

CFOLib::CFO::~CFO()
{
	//assume the destructor is destructive (could be in response to exceptions), so first force ending of dcs lock
	try
	{
		ReleaseAllBuffers();
	}
	catch(...)
	{
 		__COUT_WARN__ << "Ignoring exception caught in DESTRUCTOR while calling ReleaseAllBuffers()";
	}
	
 	__COUT_INFO__ << "DESTRUCTOR exit";
}

//
// DMA Functions
//
std::vector<std::unique_ptr<CFOLib::CFO_Event>> CFOLib::CFO::GetData(DTC_EventWindowTag when, bool matchEventWindowTag)
{
	CFO_TLOG(TLVL_GetData) << "GetData begin";
	std::vector<std::unique_ptr<CFO_Event>> output;
	std::unique_ptr<CFO_Event> packet = nullptr;
	ReleaseBuffers(CFO_DMA_Engine_DAQ);

	try
	{
		// Read the next CFO_Event
		auto tries = 0;
		while (packet == nullptr && tries < 3)
		{
			CFO_TLOG(TLVL_GetData) << "GetData before ReadNextDAQPacket, tries=" << tries;
			packet = ReadNextDAQDMA(100);
			if (packet != nullptr)
			{
				CFO_TLOG(TLVL_GetData) << "GetData after ReadDMADAQPacket, ts=0x" << std::hex
								   << packet->GetEventWindowTag().GetEventWindowTag(true);
			}
			tries++;
			// if (packet == nullptr) usleep(5000);
		}
		if (packet == nullptr)
		{
			CFO_TLOG(TLVL_GetData) << "GetData: Timeout Occurred! (CFO_Event is nullptr after retries)";
			return output;
		}

		if (packet->GetEventWindowTag() != when && matchEventWindowTag)
		{
			CFO_TLOG(TLVL_ERROR) << "GetData: Error: CFO_Event has wrong Event Window Tag! 0x" << std::hex << when.GetEventWindowTag(true)
							 << "(expected) != 0x" << std::hex << packet->GetEventWindowTag().GetEventWindowTag(true);
			packet.reset(nullptr);
			daqDMAInfo_.currentReadPtr = daqDMAInfo_.lastReadPtr;
			return output;
		}

		when = packet->GetEventWindowTag();

		CFO_TLOG(TLVL_GetData) << "GetData: Adding CFO_Event " << (void*)daqDMAInfo_.lastReadPtr << " to the list (first)";
		output.push_back(std::move(packet));

		auto done = false;
		while (!done)
		{
			CFO_TLOG(TLVL_GetData) << "GetData: Reading next DAQ Packet";
			packet = ReadNextDAQDMA(0);
			if (packet == nullptr)  // End of Data
			{
				CFO_TLOG(TLVL_GetData) << "GetData: Next packet is nullptr; we're done";
				done = true;
				daqDMAInfo_.currentReadPtr = nullptr;
			}
			else if (packet->GetEventWindowTag() != when)
			{
				CFO_TLOG(TLVL_GetData) << "GetData: Next packet has ts=0x" << std::hex << packet->GetEventWindowTag().GetEventWindowTag(true)
								   << ", not 0x" << std::hex << when.GetEventWindowTag(true) << "; we're done";
				done = true;
				daqDMAInfo_.currentReadPtr = daqDMAInfo_.lastReadPtr;
			}
			else
			{
				CFO_TLOG(TLVL_GetData) << "GetData: Next packet has same ts=0x" << std::hex
								   << packet->GetEventWindowTag().GetEventWindowTag(true) << ", continuing (bc=0x" << std::hex
								   << packet->GetEventByteCount() << ")";
			}

			if (!done)
			{
				CFO_TLOG(TLVL_GetData) << "GetData: Adding pointer " << (void*)daqDMAInfo_.lastReadPtr << " to the list";
				output.push_back(std::move(packet));
			}
		}
	}
	catch (DTC_WrongPacketTypeException& ex)
	{
		__COUT_WARN__ << "GetData: Bad omen: Wrong packet type at the current read position";
		daqDMAInfo_.currentReadPtr = nullptr;
	}
	catch (DTC_IOErrorException& ex)
	{
		daqDMAInfo_.currentReadPtr = nullptr;
		__COUT_WARN__ << "GetData: IO Exception Occurred!";
	}
	catch (DTC_DataCorruptionException& ex)
	{
		daqDMAInfo_.currentReadPtr = nullptr;
		__COUT_WARN__ << "GetData: Data Corruption Exception Occurred!";
	}

	CFO_TLOG(TLVL_GetData) << "GetData RETURN";
	return output;
}  // GetData

std::unique_ptr<CFOLib::CFO_Event> CFOLib::CFO::ReadNextDAQDMA(int tmo_ms)
{
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA BEGIN";

	if (daqDMAInfo_.currentReadPtr != nullptr)
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA BEFORE BUFFER CHECK daqDMAInfo_.currentReadPtr="
									 << (void*)daqDMAInfo_.currentReadPtr << " *nextReadPtr_=0x" << std::hex
									 << *(uint16_t*)daqDMAInfo_.currentReadPtr;
	}
	else
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA BEFORE BUFFER CHECK daqDMAInfo_.currentReadPtr=nullptr";
	}

	auto index = GetCurrentBuffer(&daqDMAInfo_);

	// Need new buffer if GetCurrentBuffer returns -1 (no buffers) or -2 (done with all held buffers)
	if (index < 0)
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA Obtaining new DAQ Buffer";

		void* oldBufferPtr = nullptr;
		if (daqDMAInfo_.buffer.size() > 0) oldBufferPtr = &daqDMAInfo_.buffer.back()[0];
		auto sts = ReadBuffer(CFO_DMA_Engine_DAQ, tmo_ms);  // does return code
		if (sts <= 0)
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA: ReadBuffer returned " << sts << ", returning nullptr";
			return nullptr;
		}
		// MUST BE ABLE TO HANDLE daqbuffer_==nullptr OR retry forever?
		daqDMAInfo_.currentReadPtr = &daqDMAInfo_.buffer.back()[0];
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA daqDMAInfo_.currentReadPtr=" << (void*)daqDMAInfo_.currentReadPtr
									 << " *daqDMAInfo_.currentReadPtr=0x" << std::hex << *(unsigned*)daqDMAInfo_.currentReadPtr
									 << " lastReadPtr_=" << (void*)daqDMAInfo_.lastReadPtr;
		void* bufferIndexPointer = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 2;
		if (daqDMAInfo_.currentReadPtr == oldBufferPtr && daqDMAInfo_.bufferIndex == *static_cast<uint32_t*>(bufferIndexPointer))
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket)
				<< "ReadNextDAQDMA: New buffer is the same as old. Releasing buffer and returning nullptr";
			daqDMAInfo_.currentReadPtr = nullptr;
			// We didn't actually get a new buffer...this probably means there's no more data
			// Try and see if we're merely stuck...hopefully, all the data is out of the buffers...
			device_.read_release(CFO_DMA_Engine_DAQ, 1);
			return nullptr;
		}
		daqDMAInfo_.bufferIndex++;

		daqDMAInfo_.currentReadPtr = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 2;
		*static_cast<uint32_t*>(daqDMAInfo_.currentReadPtr) = daqDMAInfo_.bufferIndex;
		daqDMAInfo_.currentReadPtr = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 6;

		index = daqDMAInfo_.buffer.size() - 1;
	}

	CFO_TLOG(TLVL_ReadNextDAQPacket) << "Creating CFO_Event from current DMA Buffer";
	//Utilities::PrintBuffer(daqDMAInfo_.currentReadPtr, 128, TLVL_ReadNextDAQPacket);
	auto res = std::make_unique<CFO_Event>(daqDMAInfo_.currentReadPtr);

	auto eventByteCount = res->GetEventByteCount();
	if (eventByteCount == 0) {
		throw std::runtime_error("Event inclusive byte count cannot be zero!");
	}
	size_t remainingBufferSize = GetBufferByteCount(&daqDMAInfo_, index) - sizeof(uint64_t);
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "eventByteCount: " << eventByteCount << ", remainingBufferSize: " << remainingBufferSize;
	// Check for continued DMA
	if (eventByteCount > remainingBufferSize)
	{
		// We're going to set lastReadPtr here, so that if this buffer isn't used by GetData, we start at the beginning of this event next time
		daqDMAInfo_.lastReadPtr = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) - 8;

		auto inmem = std::make_unique<CFO_Event>(eventByteCount);
		memcpy(const_cast<void*>(inmem->GetRawBufferPointer()), res->GetRawBufferPointer(), remainingBufferSize);

		auto bytes_read = remainingBufferSize;
		while (bytes_read < eventByteCount)
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA Obtaining new DAQ Buffer, bytes_read=" << bytes_read << ", eventByteCount=" << eventByteCount;

			void* oldBufferPtr = nullptr;
			if (daqDMAInfo_.buffer.size() > 0) oldBufferPtr = &daqDMAInfo_.buffer.back()[0];
			auto sts = ReadBuffer(CFO_DMA_Engine_DAQ, tmo_ms);  // does return code
			if (sts <= 0)
			{
				CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA: ReadBuffer returned " << sts << ", returning nullptr";
				return nullptr;
			}
			// MUST BE ABLE TO HANDLE daqbuffer_==nullptr OR retry forever?
			daqDMAInfo_.currentReadPtr = &daqDMAInfo_.buffer.back()[0];
			CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA daqDMAInfo_.currentReadPtr=" << (void*)daqDMAInfo_.currentReadPtr
										 << " *daqDMAInfo_.currentReadPtr=0x" << std::hex << *(unsigned*)daqDMAInfo_.currentReadPtr
										 << " lastReadPtr_=" << (void*)daqDMAInfo_.lastReadPtr;
			void* bufferIndexPointer = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 2;
			if (daqDMAInfo_.currentReadPtr == oldBufferPtr && daqDMAInfo_.bufferIndex == *static_cast<uint32_t*>(bufferIndexPointer))
			{
				CFO_TLOG(TLVL_ReadNextDAQPacket)
					<< "ReadNextDAQDMA: New buffer is the same as old. Releasing buffer and returning nullptr";
				daqDMAInfo_.currentReadPtr = nullptr;
				// We didn't actually get a new buffer...this probably means there's no more data
				// Try and see if we're merely stuck...hopefully, all the data is out of the buffers...
				device_.read_release(CFO_DMA_Engine_DAQ, 1);
				return nullptr;
			}
			daqDMAInfo_.bufferIndex++;

			size_t buffer_size = *static_cast<uint16_t*>(daqDMAInfo_.currentReadPtr);
			daqDMAInfo_.currentReadPtr = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 2;
			*static_cast<uint32_t*>(daqDMAInfo_.currentReadPtr) = daqDMAInfo_.bufferIndex;
			daqDMAInfo_.currentReadPtr = static_cast<uint8_t*>(daqDMAInfo_.currentReadPtr) + 6;

			size_t remainingEventSize = eventByteCount - bytes_read;
			size_t copySize = remainingEventSize < buffer_size - 8 ? remainingEventSize : buffer_size - 8;
			memcpy(const_cast<uint8_t*>(static_cast<const uint8_t*>(inmem->GetRawBufferPointer()) + bytes_read), daqDMAInfo_.currentReadPtr, copySize);
			bytes_read += buffer_size - 8;

			// Increment by the size of the data block
			daqDMAInfo_.currentReadPtr = reinterpret_cast<char*>(daqDMAInfo_.currentReadPtr) + copySize;
		}

		res.swap(inmem);
	}
	else {
		// Update the packet pointers

		// lastReadPtr_ is easy...
		daqDMAInfo_.lastReadPtr = daqDMAInfo_.currentReadPtr;

		// Increment by the size of the data block
		daqDMAInfo_.currentReadPtr = reinterpret_cast<char*>(daqDMAInfo_.currentReadPtr) + res->GetEventByteCount();
	}
	res->SetupEvent();
	
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextDAQDMA: RETURN";
	return res;
}

std::unique_ptr<CFOLib::CFO_DataPacket> CFOLib::CFO::ReadNextPacket(const DTC_DMA_Engine& engine, int tmo_ms)
{
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEGIN";
	DMAInfo* info;
	if (engine == CFO_DMA_Engine_DAQ)
		info = &daqDMAInfo_;
	// else if (engine == DTC_DMA_Engine_DCS)
	// 	info = &dcsDMAInfo_;
	else
	{
		__COUT_ERR__ << "ReadNextPacket: Invalid DMA Engine specified!";
		throw new DTC_DataCorruptionException();
	}

	if (info->currentReadPtr != nullptr)
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEFORE BUFFER CHECK info->currentReadPtr="
									 << (void*)info->currentReadPtr << " *nextReadPtr_=0x" << std::hex
									 << *(uint16_t*)info->currentReadPtr;
	}
	else
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEFORE BUFFER CHECK info->currentReadPtr=nullptr";
	}

	auto index = GetCurrentBuffer(info);

	// Need new buffer if GetCurrentBuffer returns -1 (no buffers) or -2 (done with all held buffers)
	if (index < 0)
	{
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket Obtaining new " << (engine == CFO_DMA_Engine_DAQ ? "DAQ" : "DCS")
									 << " Buffer";

		void* oldBufferPtr = nullptr;
		if (info->buffer.size() > 0) oldBufferPtr = &info->buffer.back()[0];
		auto sts = ReadBuffer(engine, tmo_ms);  // does return code
		if (sts <= 0)
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: ReadBuffer returned " << sts << ", returning nullptr";
			return nullptr;
		}
		// MUST BE ABLE TO HANDLE daqbuffer_==nullptr OR retry forever?
		info->currentReadPtr = &info->buffer.back()[0];
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket info->currentReadPtr=" << (void*)info->currentReadPtr
									 << " *info->currentReadPtr=0x" << std::hex << *(unsigned*)info->currentReadPtr
									 << " lastReadPtr_=" << (void*)info->lastReadPtr;
		void* bufferIndexPointer = static_cast<uint8_t*>(info->currentReadPtr) + 2;
		if (info->currentReadPtr == oldBufferPtr && info->bufferIndex == *static_cast<uint32_t*>(bufferIndexPointer))
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket)
				<< "ReadNextPacket: New buffer is the same as old. Releasing buffer and returning nullptr";
			info->currentReadPtr = nullptr;
			// We didn't actually get a new buffer...this probably means there's no more data
			// Try and see if we're merely stuck...hopefully, all the data is out of the buffers...
			device_.read_release(engine, 1);
			return nullptr;
		}
		info->bufferIndex++;

		info->currentReadPtr = reinterpret_cast<uint8_t*>(info->currentReadPtr) + 2;
		*static_cast<uint32_t*>(info->currentReadPtr) = info->bufferIndex;
		info->currentReadPtr = reinterpret_cast<uint8_t*>(info->currentReadPtr) + 6;

		index = info->buffer.size() - 1;
	}

	// Read the next packet
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket reading next packet from buffer: info->currentReadPtr="
								 << (void*)info->currentReadPtr;

	auto blockByteCount = *reinterpret_cast<uint16_t*>(info->currentReadPtr);
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: blockByteCount=" << blockByteCount
								 << ", info->currentReadPtr=" << (void*)info->currentReadPtr
								 << ", *nextReadPtr=" << (int)*((uint16_t*)info->currentReadPtr);
	if (blockByteCount == 0 || blockByteCount == 0xcafe)
	{
		if (static_cast<size_t>(index) < info->buffer.size() - 1)
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: blockByteCount is invalid, moving to next buffer";
			auto nextBufferPtr = *info->buffer[index + 1];
			info->currentReadPtr = nextBufferPtr + 8;  // Offset past DMA header
			return ReadNextPacket(engine, tmo_ms);     // Recursion
		}
		else
		{
			CFO_TLOG(TLVL_ReadNextDAQPacket)
				<< "ReadNextPacket: blockByteCount is invalid, and this is the last buffer! Returning nullptr!";
			info->currentReadPtr = nullptr;
			// This buffer is invalid, release it!
			// Try and see if we're merely stuck...hopefully, all the data is out of the buffers...
			device_.read_release(engine, 1);
			return nullptr;
		}
	}

	auto test = std::make_unique<CFO_DataPacket>(info->currentReadPtr);
	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: current+blockByteCount="
								 << (void*)(reinterpret_cast<uint8_t*>(info->currentReadPtr) + blockByteCount)
								 << ", end of dma buffer="
								 << (void*)(info->buffer[index][0] + GetBufferByteCount(info, index) +
											8);  // +8 because first 8 bytes are not included in byte count
	if (reinterpret_cast<uint8_t*>(info->currentReadPtr) + blockByteCount >
		info->buffer[index][0] + GetBufferByteCount(info, index) + 8)
	{
		blockByteCount = static_cast<uint16_t>(
			info->buffer[index][0] + GetBufferByteCount(info, index) + 8 -
			reinterpret_cast<uint8_t*>(info->currentReadPtr));  // +8 because first 8 bytes are not included in byte count
		CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: Adjusting blockByteCount to " << blockByteCount
									 << " due to end-of-DMA condition";
		test->SetByte(0, blockByteCount & 0xFF);
		test->SetByte(1, (blockByteCount >> 8));
	}

	CFO_TLOG(TLVL_ReadNextDAQPacket) << test->toJSON();

	// Update the packet pointers

	// lastReadPtr_ is easy...
	info->lastReadPtr = info->currentReadPtr;

	// Increment by the size of the data block
	info->currentReadPtr = reinterpret_cast<char*>(info->currentReadPtr) + blockByteCount;

	CFO_TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: RETURN";
	return test;
}

//
// Private Functions.
//
int CFOLib::CFO::ReadBuffer(const DTC_DMA_Engine& channel, int tmo_ms)
{
	mu2e_databuff_t* buffer;

	int retry = 1;

	// Break long timeouts into multiple 10 ms retries
	if (tmo_ms > 10)
	{
		retry = tmo_ms / 10;
		tmo_ms = 10;
	}

	int errorCode;
	do
	{
		CFO_TLOG(TLVL_ReadBuffer) << "ReadBuffer before device_.read_data tmo=" << tmo_ms << " retry=" << retry;
		errorCode = device_.read_data(channel, reinterpret_cast<void**>(&buffer), 1);
		// if (errorCode == 0) usleep(1000);

	} while (retry-- > 0 && errorCode == 0);  //error code of 0 is timeout

	if (errorCode == 0)
	{
		CFO_TLOG(TLVL_ReadBuffer) << "ReadBuffer: Device timeout occurred! ec=" << errorCode << ", rt=" << retry;
	}
	else if (errorCode < 0)
	{
		CFO_TLOG(TLVL_ERROR) << "ReadBuffer: read_data returned " << errorCode << ", throwing CFO_IOErrorException!";
		throw DTC_IOErrorException(errorCode);
	}
	else
	{
		CFO_TLOG(TLVL_ReadBuffer) << "ReadBuffer buffer_=" << (void*)buffer << " errorCode=" << errorCode << " *buffer_=0x"
							  << std::hex << *(unsigned*)buffer;
		if (channel == CFO_DMA_Engine_DAQ)
		{
			daqDMAInfo_.buffer.push_back(buffer);
			CFO_TLOG(TLVL_ReadBuffer) << "ReadBuffer: There are now " << daqDMAInfo_.buffer.size()
								  << " DAQ buffers held in the DTC Library";
		}
		// else if (channel == DTC_DMA_Engine_DCS)
		// {
		// 	dcsDMAInfo_.buffer.push_back(buffer);
		// 	DTC_TLOG(TLVL_ReadBuffer) << "ReadBuffer: There are now " << dcsDMAInfo_.buffer.size()
		// 						  << " DCS buffers held in the DTC Library";
		// }
	}
	return errorCode;
}

void CFOLib::CFO::ReleaseBuffers(const DTC_DMA_Engine& channel)
{
	CFO_TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers BEGIN";
	DMAInfo* info;
	if (channel == CFO_DMA_Engine_DAQ)
		info = &daqDMAInfo_;
	// else if (channel == DTC_DMA_Engine_DCS)
	// 	info = &dcsDMAInfo_;
	else
	{
		CFO_TLOG(TLVL_ERROR) << "ReadNextPacket: Invalid DMA Engine specified!";
		throw new DTC_DataCorruptionException();
	}

	auto releaseBufferCount = GetCurrentBuffer(info);
	if (releaseBufferCount > 0)
	{
		CFO_TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers releasing " << releaseBufferCount << " "
								  << (channel == CFO_DMA_Engine_DAQ ? "DAQ" : "DCS") << " buffers.";

		// if (channel == CFO_DMA_Engine_DCS)
		// 	device_.begin_dcs_transaction();
		device_.read_release(channel, releaseBufferCount);
		// if (channel == DTC_DMA_Engine_DCS)
		// 	device_.end_dcs_transaction();

		for (int ii = 0; ii < releaseBufferCount; ++ii)
		{
			info->buffer.pop_front();
		}
	}
	else
	{
		CFO_TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers releasing ALL " << (channel == CFO_DMA_Engine_DAQ ? "DAQ" : "DCS")
								  << " buffers.";
		ReleaseAllBuffers(channel);
	}
	CFO_TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers END";
}

int CFOLib::CFO::GetCurrentBuffer(DMAInfo* info)
{
	CFO_TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer BEGIN";
	if (info->currentReadPtr == nullptr || info->buffer.size() == 0)
	{
		CFO_TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer returning -1 because not currently reading a buffer";
		return -1;
	}

	for (size_t ii = 0; ii < info->buffer.size(); ++ii)
	{
		auto bufferptr = *info->buffer[ii];
		uint16_t bufferSize = *reinterpret_cast<uint16_t*>(bufferptr);
		if (info->currentReadPtr > bufferptr &&
			info->currentReadPtr < bufferptr + bufferSize)
		{
			CFO_TLOG(TLVL_GetCurrentBuffer) << "Found matching buffer at index " << ii << ".";
			return ii;
		}
	}
	CFO_TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer returning -2: Have buffers but none match, need new";
	return -2;
}

uint16_t CFOLib::CFO::GetBufferByteCount(DMAInfo* info, size_t index)
{
	if (index >= info->buffer.size()) return 0;
	auto bufferptr = *info->buffer[index];
	uint16_t bufferSize = *reinterpret_cast<uint16_t*>(bufferptr);
	return bufferSize;
}

