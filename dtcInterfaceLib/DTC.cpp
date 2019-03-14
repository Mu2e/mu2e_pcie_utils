#define TRACE_NAME "DTC"
#include "trace.h"

#define TLVL_GetData 10
#define TLVL_GetJSONData 11
#define TLVL_ReadBuffer 12
#define TLVL_ReadNextDAQPacket 13
#define TLVL_ReadNextDCSPacket 14
#define TLVL_SendDCSRequestPacket 15
#define TLVL_SendReadoutRequestPacket 16
#define TLVL_VerifySimFileInDTC 17
#define TLVL_VerifySimFileInDTC2 18
#define TLVL_VerifySimFileInDTC3 19
#define TLVL_WriteSimFileToDTC 20
#define TLVL_WriteSimFileToDTC2 21
#define TLVL_WriteSimFileToDTC3 22
#define TLVL_WriteDetectorEmulatorData 23
#define TLVL_WriteDataPacket 24
#define TLVL_ReleaseBuffers 25
#define TLVL_GetCurrentBuffer 26

#include "DTC.h"

#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>  // Convert uint to hex string

DTCLib::DTC::DTC(DTC_SimMode mode, int dtc, unsigned rocMask, std::string expectedDesignVersion)
	: DTC_Registers(mode, dtc, rocMask, expectedDesignVersion), daqDMAInfo_(), dcsDMAInfo_()
{
	// ELF, 05/18/2016: Rick reports that 3.125 Gbp
	// SetSERDESOscillatorClock(DTC_SerdesClockSpeed_25Gbps); // We're going to 2.5Gbps for now
	TLOG(TLVL_INFO) << "CONSTRUCTOR";
}

DTCLib::DTC::~DTC()
{
	TLOG(TLVL_INFO) << "DESTRUCTOR";
	ReleaseAllBuffers();
}

//
// DMA Functions
//
std::vector<DTCLib::DTC_DataBlock> DTCLib::DTC::GetData(DTC_Timestamp when)
{
	TLOG(TLVL_GetData) << "GetData begin";
	std::vector<DTC_DataBlock> output;
	std::unique_ptr<DTC_DataHeaderPacket> packet = nullptr;
	ReleaseBuffers(DTC_DMA_Engine_DAQ);

	try
	{
		// Read the header packet
		auto tries = 0;
		size_t sz;
		while (packet == nullptr && tries < 3) {
			TLOG(TLVL_GetData) << "GetData before ReadNextDAQPacket, tries=" << tries;
			packet = ReadNextDAQPacket(100);
			if (packet != nullptr) {
				TLOG(TLVL_GetData) << "GetData after ReadDMADAQPacket, ts=0x" << std::hex
								   << packet->GetTimestamp().GetTimestamp(true);
			}
			tries++;
			// if (packet == nullptr) usleep(5000);
		}
		if (packet == nullptr) {
			TLOG(TLVL_GetData) << "GetData: Timeout Occurred! (Lead packet is nullptr after retries)";
			return output;
		}

		if (packet->GetTimestamp() != when && when.GetTimestamp(true) != 0) {
			TLOG(TLVL_ERROR) << "GetData: Error: Lead packet has wrong timestamp! 0x" << std::hex << when.GetTimestamp(true)
							 << "(expected) != 0x" << std::hex << packet->GetTimestamp().GetTimestamp(true);
			packet.reset(nullptr);
			daqDMAInfo_.currentReadPtr = daqDMAInfo_.lastReadPtr;
			return output;
		}

		sz = packet->GetByteCount();
		when = packet->GetTimestamp();

		packet.reset(nullptr);

		TLOG(TLVL_GetData) << "GetData: Adding pointer " << (void*)daqDMAInfo_.lastReadPtr << " to the list (first)";
		output.push_back(DTC_DataBlock(reinterpret_cast<DTC_DataBlock::pointer_t*>(daqDMAInfo_.lastReadPtr), sz));

		auto done = false;
		while (!done) {
			size_t sz2 = 0;
			TLOG(TLVL_GetData) << "GetData: Reading next DAQ Packet";
			packet = ReadNextDAQPacket();
			if (packet == nullptr)  // End of Data
			{
				TLOG(TLVL_GetData) << "GetData: Next packet is nullptr; we're done";
				done = true;
				daqDMAInfo_.currentReadPtr = nullptr;
			}
			else if (packet->GetTimestamp() != when)
			{
				TLOG(TLVL_GetData) << "GetData: Next packet has ts=0x" << std::hex << packet->GetTimestamp().GetTimestamp(true)
								   << ", not 0x" << std::hex << when.GetTimestamp(true) << "; we're done";
				done = true;
				daqDMAInfo_.currentReadPtr = daqDMAInfo_.lastReadPtr;
			}
			else
			{
				TLOG(TLVL_GetData) << "GetData: Next packet has same ts=0x" << std::hex
								   << packet->GetTimestamp().GetTimestamp(true) << ", continuing (bc=0x" << std::hex
								   << packet->GetByteCount() << ")";
				sz2 = packet->GetByteCount();
			}
			packet.reset(nullptr);

			if (!done) {
				TLOG(TLVL_GetData) << "GetData: Adding pointer " << (void*)daqDMAInfo_.lastReadPtr << " to the list";
				output.push_back(DTC_DataBlock(reinterpret_cast<DTC_DataBlock::pointer_t*>(daqDMAInfo_.lastReadPtr), sz2));
			}
		}
	}
	catch (DTC_WrongPacketTypeException& ex)
	{
		TLOG(TLVL_WARNING) << "GetData: Bad omen: Wrong packet type at the current read position";
		daqDMAInfo_.currentReadPtr = nullptr;
	}
	catch (DTC_IOErrorException& ex)
	{
		daqDMAInfo_.currentReadPtr = nullptr;
		TLOG(TLVL_WARNING) << "GetData: IO Exception Occurred!";
	}
	catch (DTC_DataCorruptionException& ex)
	{
		daqDMAInfo_.currentReadPtr = nullptr;
		TLOG(TLVL_WARNING) << "GetData: Data Corruption Exception Occurred!";
	}

	TLOG(TLVL_GetData) << "GetData RETURN";
	packet.reset(nullptr);
	return output;
}  // GetData

std::string DTCLib::DTC::GetJSONData(DTC_Timestamp when)
{
	TLOG(TLVL_GetJSONData) << "GetJSONData BEGIN";
	std::stringstream ss;
	TLOG(TLVL_GetJSONData) << "GetJSONData before call to GetData";
	auto data = GetData(when);
	TLOG(TLVL_GetJSONData) << "GetJSONData after call to GetData, data size " << data.size();

	for (size_t i = 0; i < data.size(); ++i) {
		TLOG(TLVL_GetJSONData) << "GetJSONData constructing DataPacket:";
		auto test = DTC_DataPacket(data[i].blockPointer);
		TLOG(TLVL_GetJSONData) << test.toJSON();
		TLOG(TLVL_GetJSONData) << "GetJSONData constructing DataHeaderPacket";
		auto theHeader = DTC_DataHeaderPacket(test);
		ss << "DataBlock: {";
		ss << theHeader.toJSON() << ",";
		ss << "DataPackets: [";
		for (auto packet = 0; packet < theHeader.GetPacketCount(); ++packet) {
			auto packetPtr = static_cast<void*>(reinterpret_cast<char*>(data[i].blockPointer) + 16 * (1 + packet));
			ss << DTC_DataPacket(packetPtr).toJSON() << ",";
		}
		ss << "]";
		ss << "}";
		if (i + 1 < data.size()) {
			ss << ",";
		}
	}

	TLOG(TLVL_GetJSONData) << "GetJSONData RETURN";
	return ss.str();
}

void DTCLib::DTC::WriteSimFileToDTC(std::string file, bool /*goForever*/, bool overwriteEnvironment,
									std::string outputFileName, bool skipVerify)
{
	bool success = false;
	int retryCount = 0;
	while (!success && retryCount < 5) {
		TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC BEGIN";
		auto writeOutput = outputFileName != "";
		std::ofstream outputStream;
		if (writeOutput) {
			outputStream.open(outputFileName, std::ios::out | std::ios::binary);
		}
		auto sim = getenv("DTCLIB_SIM_FILE");
		if (!overwriteEnvironment && sim != nullptr) {
			file = std::string(sim);
		}

		TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC file is " << file << ", Setting up DTC";
		DisableDetectorEmulator();
		DisableDetectorEmulatorMode();
		// ResetDDR();  // this can take about a second
		ResetDDRWriteAddress();
		ResetDDRReadAddress();
		SetDDRDataLocalStartAddress(0x0);
		SetDDRDataLocalEndAddress(0xFFFFFFFF);
		EnableDetectorEmulatorMode();
		SetDetectorEmulationDMACount(1);
		SetDetectorEmulationDMADelayCount(250);  // 1 microseconds
		uint64_t totalSize = 0;
		auto n = 0;

		auto sizeCheck = true;
		TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC Opening file";
		std::ifstream is(file, std::ifstream::binary);
		TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC Reading file";
		while (is && is.good() && sizeCheck) {
			TLOG(TLVL_WriteSimFileToDTC2) << "WriteSimFileToDTC Reading a DMA from file..." << file;
			auto buf = reinterpret_cast<mu2e_databuff_t*>(new char[0x10000]);
			is.read(reinterpret_cast<char*>(buf), sizeof(uint64_t));
			if (is.eof()) {
				TLOG(TLVL_WriteSimFileToDTC2) << "WriteSimFileToDTC End of file reached.";
				delete[] buf;
				break;
			}
			auto sz = *reinterpret_cast<uint64_t*>(buf);
			// TLOG(5) << "Size is " << << ", writing to device", (long long unsigned)sz);
			is.read(reinterpret_cast<char*>(buf) + 8, sz - sizeof(uint64_t));
			if (sz < 80 && sz > 0) {
				auto oldSize = sz;
				sz = 80;
				memcpy(buf, &sz, sizeof(uint64_t));
				uint64_t sixtyFour = 64;
				memcpy(reinterpret_cast<uint64_t*>(buf) + 1, &sixtyFour, sizeof(uint64_t));
				bzero(reinterpret_cast<uint64_t*>(buf) + 2, sz - oldSize);
			}
			// is.read((char*)buf + 8, sz - sizeof(uint64_t));
			if (sz > 0 && (sz + totalSize < 0xFFFFFFFF || simMode_ == DTC_SimMode_LargeFile)) {
				TLOG(TLVL_WriteSimFileToDTC2) << "WriteSimFileToDTC Size is " << sz << ", writing to device";
				if (writeOutput) {
					TLOG(TLVL_WriteSimFileToDTC3)
						<< "WriteSimFileToDTC: Stripping off DMA header words and writing to binary file";
					outputStream.write(reinterpret_cast<char*>(buf) + 16, sz - 16);
				}

				auto exclusiveByteCount = *(reinterpret_cast<uint64_t*>(buf) + 1);
				TLOG(TLVL_WriteSimFileToDTC3) << "WriteSimFileToDTC: Inclusive byte count: " << sz
											  << ", Exclusive byte count: " << exclusiveByteCount;
				if (sz - 16 != exclusiveByteCount) {
					TLOG(TLVL_ERROR) << "WriteSimFileToDTC: ERROR: Inclusive Byte count " << sz
									 << " is inconsistent with exclusive byte count " << exclusiveByteCount << " for DMA at 0x"
									 << std::hex << totalSize << " (" << sz - 16 << " != " << exclusiveByteCount << ")";
					sizeCheck = false;
				}

				totalSize += sz - 8;
				n++;
				TLOG(TLVL_WriteSimFileToDTC3) << "WriteSimFileToDTC: totalSize is now " << totalSize << ", n is now " << n;
				WriteDetectorEmulatorData(buf, static_cast<size_t>(sz));
			}
			else if (sz > 0)
			{
				TLOG(TLVL_WriteSimFileToDTC2) << "WriteSimFileToDTC DTC memory is now full. Closing file.";
				sizeCheck = false;
			}
			delete[] buf;
		}

		TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC Closing file. sizecheck=" << sizeCheck << ", eof=" << is.eof()
									 << ", fail=" << is.fail() << ", bad=" << is.bad();
		is.close();
		if (writeOutput) outputStream.close();
		SetDDRDataLocalEndAddress(static_cast<uint32_t>(totalSize - 1));
		success = skipVerify || VerifySimFileInDTC(file, outputFileName);
		retryCount++;
	}

	if (retryCount == 5) {
		TLOG(TLVL_ERROR) << "WriteSimFileToDTC FAILED after 5 attempts! ABORTING!";
		exit(4);
	}
	else
	{
		TLOG(TLVL_INFO) << "WriteSimFileToDTC Took " << retryCount << " attempts to write file";
	}

	SetDetectorEmulatorInUse();
	TLOG(TLVL_WriteSimFileToDTC) << "WriteSimFileToDTC END";
}

bool DTCLib::DTC::VerifySimFileInDTC(std::string file, std::string rawOutputFilename)
{
	uint64_t totalSize = 0;
	auto n = 0;
	auto sizeCheck = true;

	auto writeOutput = rawOutputFilename != "";
	std::ofstream outputStream;
	if (writeOutput) {
		outputStream.open(rawOutputFilename + ".verify", std::ios::out | std::ios::binary);
	}

	auto sim = getenv("DTCLIB_SIM_FILE");
	if (file.size() == 0 && sim != nullptr) {
		file = std::string(sim);
	}

	ResetDDRReadAddress();
	TLOG(TLVL_VerifySimFileInDTC) << "VerifySimFileInDTC Opening file";
	std::ifstream is(file, std::ifstream::binary);
	if (!is || !is.good()) {
		TLOG(TLVL_ERROR) << "VerifySimFileInDTC Failed to open file " << file << "!";
	}

	TLOG(TLVL_VerifySimFileInDTC) << "VerifySimFileInDTC Reading file";
	while (is && is.good() && sizeCheck) {
		TLOG(TLVL_VerifySimFileInDTC2) << "VerifySimFileInDTC Reading a DMA from file..." << file;
		auto buf = reinterpret_cast<mu2e_databuff_t*>(new char[0x10000]);
		is.read(reinterpret_cast<char*>(buf), sizeof(uint64_t));
		if (is.eof()) {
			TLOG(TLVL_VerifySimFileInDTC2) << "VerifySimFileInDTC End of file reached.";
			delete[] buf;
			break;
		}
		auto sz = *reinterpret_cast<uint64_t*>(buf);
		// TLOG(5) << "Size is " << << ", writing to device", (long long unsigned)sz);
		is.read(reinterpret_cast<char*>(buf) + 8, sz - sizeof(uint64_t));
		if (sz < 80 && sz > 0) {
			auto oldSize = sz;
			sz = 80;
			memcpy(buf, &sz, sizeof(uint64_t));
			uint64_t sixtyFour = 64;
			memcpy(reinterpret_cast<uint64_t*>(buf) + 1, &sixtyFour, sizeof(uint64_t));
			bzero(reinterpret_cast<uint64_t*>(buf) + 2, sz - oldSize);
		}

		if (sz > 0 && (sz + totalSize < 0xFFFFFFFF || simMode_ == DTC_SimMode_LargeFile)) {
			TLOG(TLVL_VerifySimFileInDTC2) << "VerifySimFileInDTC Expected Size is " << sz << ", reading from device";
			auto exclusiveByteCount = *(reinterpret_cast<uint64_t*>(buf) + 1);
			TLOG(TLVL_VerifySimFileInDTC3) << "VerifySimFileInDTC: Inclusive byte count: " << sz
										   << ", Exclusive byte count: " << exclusiveByteCount;
			if (sz - 16 != exclusiveByteCount) {
				TLOG(TLVL_ERROR) << "VerifySimFileInDTC: ERROR: Inclusive Byte count " << sz
								 << " is inconsistent with exclusive byte count " << exclusiveByteCount << " for DMA at 0x"
								 << std::hex << totalSize << " (" << sz - 16 << " != " << exclusiveByteCount << ")";
				sizeCheck = false;
			}

			totalSize += sz;
			n++;
			TLOG(TLVL_VerifySimFileInDTC3) << "VerifySimFileInDTC: totalSize is now " << totalSize << ", n is now " << n;
			// WriteDetectorEmulatorData(buf, static_cast<size_t>(sz));
			DisableDetectorEmulator();
			SetDetectorEmulationDMACount(1);
			EnableDetectorEmulator();

			mu2e_databuff_t* buffer;
			auto tmo_ms = 1500;
			TLOG(TLVL_VerifySimFileInDTC) << "VerifySimFileInDTC - before read for DAQ ";
			auto sts = device_.read_data(DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
			if (writeOutput && sts > 8) {
				TLOG(TLVL_VerifySimFileInDTC3) << "VerifySimFileInDTC: Writing to binary file";
				outputStream.write(reinterpret_cast<char*>(*buffer + 8), sts - 8);
			}
			size_t readSz = *(reinterpret_cast<uint64_t*>(buffer));
			TLOG(TLVL_VerifySimFileInDTC) << "VerifySimFileInDTC - after read, sz=" << sz << " sts=" << sts
										  << " rdSz=" << readSz;

			// DMA engine strips off leading 64-bit word
			TLOG(6) << "VerifySimFileInDTC - Checking buffer size";
			if (static_cast<size_t>(sts) != sz - sizeof(uint64_t)) {
				TLOG(TLVL_ERROR) << "VerifySimFileInDTC Buffer " << n << " has size 0x" << std::hex << sts
								 << " but the input file has size 0x" << std::hex << sz - sizeof(uint64_t)
								 << " for that buffer!";

				device_.read_release(DTC_DMA_Engine_DAQ, 1);
				delete[] buf;
				is.close();
				if (writeOutput) outputStream.close();
				return false;
			}

			TLOG(TLVL_VerifySimFileInDTC2) << "VerifySimFileInDTC - Checking buffer contents";
			size_t cnt = sts % sizeof(uint64_t) == 0 ? sts / sizeof(uint64_t) : 1 + (sts / sizeof(uint64_t));

			for (size_t ii = 0; ii < cnt; ++ii) {
				auto l = *(reinterpret_cast<uint64_t*>(*buffer) + ii);
				auto r = *(reinterpret_cast<uint64_t*>(*buf) + ii + 1);
				if (l != r) {
					size_t address = totalSize - sz + ((ii + 1) * sizeof(uint64_t));
					TLOG(TLVL_ERROR) << "VerifySimFileInDTC Buffer " << n << " word " << ii << " (Address in file 0x" << std::hex
									 << address << "):"
									 << " Expected 0x" << std::hex << r << ", but got 0x" << std::hex << l
									 << ". Returning False!";
					delete[] buf;
					is.close();
					if (writeOutput) outputStream.close();
					device_.read_release(DTC_DMA_Engine_DAQ, 1);
					return false;
				}
			}
			device_.read_release(DTC_DMA_Engine_DAQ, 1);
		}
		else if (sz > 0)
		{
			TLOG(TLVL_VerifySimFileInDTC2) << "VerifySimFileInDTC DTC memory is now full. Closing file.";
			sizeCheck = false;
		}
		delete[] buf;
	}

	TLOG(TLVL_VerifySimFileInDTC) << "VerifySimFileInDTC Closing file. sizecheck=" << sizeCheck << ", eof=" << is.eof()
								  << ", fail=" << is.fail() << ", bad=" << is.bad();
	TLOG(TLVL_INFO) << "VerifySimFileInDTC: The Detector Emulation file was written correctly";
	is.close();
	if (writeOutput) outputStream.close();
	return true;
}

// ROC Register Functions
uint16_t DTCLib::DTC::ReadROCRegister(const DTC_Link_ID& link, const uint16_t address, int retries)
{
	dcsDMAInfo_.currentReadPtr = nullptr;
	ReleaseBuffers(DTC_DMA_Engine_DCS);
	SendDCSRequestPacket(link, DTC_DCSOperationType_Read, address,
			 0x0 /*data*/, 0x0 /*address2*/, 0x0 /*data2*/,
										false  /*quiet*/);

	usleep(2500);
	uint16_t data = 0xFFFF;
	while (retries > 0) {
		TLOG(TLVL_TRACE) << "ReadROCRegister: Loop start, retries=" << retries;
		auto reply = ReadNextDCSPacket();
		auto count = 0;
		while (reply != nullptr) {
			count++;
			auto replytmp = reply->GetReply(false);
			auto linktmp = reply->GetRingID();
			TLOG(TLVL_TRACE) << "Got packet, "
							 << "link=" << static_cast<int>(linktmp) << " (expected " << static_cast<int>(link) << "), "
							 << "address=" << static_cast<int>(replytmp.first) << " (expected " << static_cast<int>(address)
							 << "), "
							 << "data=" << static_cast<int>(replytmp.second);

			reply.reset(nullptr);
			if (replytmp.first != address || linktmp != link) {
				TLOG(TLVL_TRACE) << "Address or link did not match, reading next packet!";
				reply = ReadNextDCSPacket();  // Read the next packet
				continue;
			}

			data = replytmp.second;
			retries = 0;
			reply = ReadNextDCSPacket();  // Read the next packet
		}
		if (count == 0) {
			SendDCSRequestPacket(link, DTC_DCSOperationType_Read, address);
			usleep(2500);
		}
		retries--;
	}
	TLOG(TLVL_TRACE) << "ReadROCRegister returning " << static_cast<int>(data) << " for link " << static_cast<int>(link)
					 << ", address " << static_cast<int>(address);
	return data;
}

void DTCLib::DTC::WriteROCRegister(const DTC_Link_ID& link, const uint16_t address, const uint16_t data)
{
	SendDCSRequestPacket(link, DTC_DCSOperationType_Write, address, data,
			0x0 /*address2*/, 0x0 /*data2*/,
			false  /*quiet*/);
}

std::pair<uint16_t, uint16_t> DTCLib::DTC::ReadROCRegisters(const DTC_Link_ID& link, const uint16_t address1,
															const uint16_t address2, int retries)
{
	dcsDMAInfo_.currentReadPtr = nullptr;
	ReleaseBuffers(DTC_DMA_Engine_DCS);
	SendDCSRequestPacket(link, DTC_DCSOperationType_Read, address1, 0, address2);
	usleep(2500);
	uint16_t data1 = 0xFFFF;
	uint16_t data2 = 0xFFFF;
	while (retries > 0) {
		TLOG(TLVL_TRACE) << "ReadROCRegister: Loop start, retries=" << retries;
		auto reply = ReadNextDCSPacket();
		auto count = 0;
		while (reply != nullptr) {
			count++;
			auto reply1tmp = reply->GetReply(false);
			auto reply2tmp = reply->GetReply(true);
			auto linktmp = reply->GetRingID();
			TLOG(TLVL_TRACE) << "Got packet, "
							 << "link=" << static_cast<int>(linktmp) << " (expected " << static_cast<int>(link) << "), "
							 << "address1=" << static_cast<int>(reply1tmp.first) << " (expected "
							 << static_cast<int>(address1) << "), "
							 << "data1=" << static_cast<int>(reply1tmp.second)
							 << "address2=" << static_cast<int>(reply2tmp.first) << " (expected "
							 << static_cast<int>(address2) << "), "
							 << "data2=" << static_cast<int>(reply2tmp.second);

			reply.reset(nullptr);
			if (reply1tmp.first != address1 || reply2tmp.first != address2 || linktmp != link) {
				TLOG(TLVL_TRACE) << "Address or link did not match, reading next packet!";
				reply = ReadNextDCSPacket();  // Read the next packet
				continue;
			}

			data1 = reply1tmp.second;
			data2 = reply2tmp.second;

			retries = 0;
			reply = ReadNextDCSPacket();  // Read the next packet
		}
		if (count == 0) {
			SendDCSRequestPacket(link, DTC_DCSOperationType_Read, address1, 0, address2);
			usleep(2500);
		}
		retries--;
	}
	TLOG(TLVL_TRACE) << "ReadROCRegister returning " << static_cast<int>(data1) << " for link " << static_cast<int>(link)
					 << ", address " << static_cast<int>(address1) << ", " << static_cast<int>(data2) << ", address "
					 << static_cast<int>(address2);
	return std::make_pair(data1, data2);
}

void DTCLib::DTC::WriteROCRegisters(const DTC_Link_ID& link, const uint16_t address1, const uint16_t data1,
									const uint16_t address2, const uint16_t data2)
{
	SendDCSRequestPacket(link, DTC_DCSOperationType_Write, address1, data1, address2, data2);
}

std::vector<uint16_t> DTCLib::DTC::ReadROCBlock(const DTC_Link_ID& link, const uint16_t address,
												const uint16_t wordCount)
{
	DTC_DCSRequestPacket req(link, DTC_DCSOperationType_BlockRead, false, address, wordCount);

	TLOG(TLVL_SendDCSRequestPacket) << "ReadROCBlock before WriteDMADCSPacket - DTC_DCSRequestPacket";

	dcsDMAInfo_.currentReadPtr = nullptr;
	ReleaseBuffers(DTC_DMA_Engine_DCS);

	if (!ReadDCSReception()) EnableDCSReception();

	WriteDMAPacket(req);
	TLOG(TLVL_SendDCSRequestPacket) << "ReadROCBlock after  WriteDMADCSPacket - DTC_DCSRequestPacket";

	usleep(2500);
	std::vector<uint16_t> data;

	auto reply = ReadNextDCSPacket();
	while (reply != nullptr) {
		auto replytmp = reply->GetReply(false);
		auto linktmp = reply->GetRingID();
		TLOG(TLVL_TRACE) << "Got packet, "
						 << "link=" << static_cast<int>(linktmp) << " (expected " << static_cast<int>(link) << "), "
						 << "address=" << static_cast<int>(replytmp.first) << " (expected " << static_cast<int>(address)
						 << "), "
						 << "wordCount=" << static_cast<int>(replytmp.second);

		data = reply->GetBlockReadData();
		auto packetCount = reply->GetBlockPacketCount();
		reply.reset(nullptr);
		if (replytmp.first != address || linktmp != link) {
			TLOG(TLVL_TRACE) << "Address or link did not match, reading next packet!";
			reply = ReadNextDCSPacket();  // Read the next packet
			continue;
		}

		auto wordCount = replytmp.second;
		auto processedWords = 3;
		while (packetCount > 0) {
			auto dataPacket = ReadNextPacket(DTC_DMA_Engine_DCS);
			if (dataPacket == nullptr) break;
			auto byteInPacket = 0;

			while (wordCount - processedWords > 0 && byteInPacket < 16) {
				uint16_t thisWord = dataPacket->GetWord(byteInPacket) + (dataPacket->GetWord(byteInPacket + 1) << 8);
				byteInPacket += 2;
				data.push_back(thisWord);
				processedWords++;
			}

			packetCount--;
		}
	}

	TLOG(TLVL_TRACE) << "ReadROCBlock returning " << static_cast<int>(data.size()) << " words for link " << static_cast<int>(link)
					 << ", address " << static_cast<int>(address);
	return data;
}

void DTCLib::DTC::WriteROCBlock(const DTC_Link_ID& link, const uint16_t address,
								const std::vector<uint16_t>& blockData)
{
	DTC_DCSRequestPacket req(link, DTC_DCSOperationType_BlockWrite, false, address);
	req.SetBlockWriteData(blockData);

	TLOG(TLVL_SendDCSRequestPacket) << "WriteROCBlock before WriteDMADCSPacket - DTC_DCSRequestPacket";

	if (!ReadDCSReception()) EnableDCSReception();

	WriteDMAPacket(req);
	TLOG(TLVL_SendDCSRequestPacket) << "WriteROCBlock after  WriteDMADCSPacket - DTC_DCSRequestPacket";
}

uint16_t DTCLib::DTC::ReadExtROCRegister(const DTC_Link_ID& link, const uint16_t block,
		const uint16_t address)
{
	uint16_t addressT = address & 0x7FFF;
	WriteROCRegister(link, 12, block);
	WriteROCRegister(link, 13, addressT);
	WriteROCRegister(link, 13, addressT | 0x8000);
	return ReadROCRegister(link, 22);
}

void DTCLib::DTC::WriteExtROCRegister(const DTC_Link_ID& link, const uint16_t block,
		const uint16_t address,
									  const uint16_t data)
{
	uint16_t dataT = data & 0x7FFF;
	WriteROCRegister(link, 12, block + (address << 8));
	WriteROCRegister(link, 13, dataT);
	WriteROCRegister(link, 13, dataT | 0x8000);
}

std::string DTCLib::DTC::ROCRegDump(const DTC_Link_ID& link)
{
	std::ostringstream o;
	o.setf(std::ios_base::boolalpha);
	o << "{";
	o << "\"Forward Detector 0 Status\": " << ReadExtROCRegister(link, 8, 0) << ",\n";
	o << "\"Forward Detector 1 Status\": " << ReadExtROCRegister(link, 9, 0) << ",\n";
	o << "\"Command Handler Status\": " << ReadExtROCRegister(link, 10, 0) << ",\n";
	o << "\"Packet Sender 0 Status\": " << ReadExtROCRegister(link, 11, 0) << ",\n";
	o << "\"Packet Sender 1 Status\": " << ReadExtROCRegister(link, 12, 0) << ",\n";
	o << "\"Forward Detector 0 Errors\": " << ReadExtROCRegister(link, 8, 1) << ",\n";
	o << "\"Forward Detector 1 Errors\": " << ReadExtROCRegister(link, 9, 1) << ",\n";
	o << "\"Command Handler Errors\": " << ReadExtROCRegister(link, 10, 1) << ",\n";
	o << "\"Packet Sender 0 Errors\": " << ReadExtROCRegister(link, 11, 1) << ",\n";
	o << "\"Packet Sender 1 Errors\": " << ReadExtROCRegister(link, 12, 1) << "\n";
	o << "}";

	return o.str();
}

void DTCLib::DTC::SendReadoutRequestPacket(const DTC_Link_ID& link, const DTC_Timestamp& when, bool quiet)
{
	DTC_HeartbeatPacket req(link, when);
	TLOG(TLVL_SendReadoutRequestPacket) << "SendReadoutRequestPacket before WriteDMADAQPacket - DTC_HeartbeatPacket";
	if (!quiet) std::cout << req.toJSON() << std::endl;
	WriteDMAPacket(req);
	TLOG(TLVL_SendReadoutRequestPacket) << "SendReadoutRequestPacket after  WriteDMADAQPacket - DTC_HeartbeatPacket";
}

void DTCLib::DTC::SendDCSRequestPacket(const DTC_Link_ID& link, const DTC_DCSOperationType type, const uint16_t address,
									   const uint16_t data, const uint16_t address2, const uint16_t data2, bool quiet)
{
	DTC_DCSRequestPacket req(link, type, false /*requestAck*/, address, data);

	if (!quiet) std::cout << "Init DCS Packet: \n" << req.toJSON() << std::endl;

	if (type == DTC_DCSOperationType_DoubleRead ||
			type == DTC_DCSOperationType_DoubleWrite)
	{
		std::cout << "Double operation enabled!" << std::endl;
		req.AddRequest(address2, data2);
	}

	TLOG(TLVL_SendDCSRequestPacket) << "SendDCSRequestPacket before WriteDMADCSPacket - DTC_DCSRequestPacket";

	if (!quiet) std::cout << "Sending DCS Packet: \n" << req.toJSON() << std::endl;

	if (!ReadDCSReception()) EnableDCSReception();

	WriteDMAPacket(req);
	TLOG(TLVL_SendDCSRequestPacket) << "SendDCSRequestPacket after  WriteDMADCSPacket - DTC_DCSRequestPacket";
}

std::unique_ptr<DTCLib::DTC_DataHeaderPacket> DTCLib::DTC::ReadNextDAQPacket(int tmo_ms)
{
	auto test = ReadNextPacket(DTC_DMA_Engine_DAQ, tmo_ms);
	if (test == nullptr) return nullptr;  // Couldn't read new block
	auto output = std::make_unique<DTC_DataHeaderPacket>(*test.get());
	TLOG(TLVL_ReadNextDAQPacket) << output->toJSON();
	if (static_cast<uint16_t>((1 + output->GetPacketCount()) * 16) != output->GetByteCount()) {
		TLOG(TLVL_ERROR) << "Data Error Detected: PacketCount: " << output->GetPacketCount()
						 << ", ExpectedByteCount: " << (1u + output->GetPacketCount()) * 16u
						 << ", BlockByteCount: " << output->GetByteCount();
		throw DTC_DataCorruptionException();
	}
	return output;
}

std::unique_ptr<DTCLib::DTC_DCSReplyPacket> DTCLib::DTC::ReadNextDCSPacket()
{
	auto test = ReadNextPacket(DTC_DMA_Engine_DCS);
	if (test == nullptr) return nullptr;  // Couldn't read new block
	auto output = std::make_unique<DTC_DCSReplyPacket>(*test.get());

	return output;
}

std::unique_ptr<DTCLib::DTC_DataPacket> DTCLib::DTC::ReadNextPacket(const DTC_DMA_Engine& engine, int tmo_ms)
{
	TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEGIN";
	DMAInfo* info;
	if (engine == DTC_DMA_Engine_DAQ)
		info = &daqDMAInfo_;
	else if (engine == DTC_DMA_Engine_DCS)
		info = &dcsDMAInfo_;
	else
	{
		TLOG(TLVL_ERROR) << "ReadNextPacket: Invalid DMA Engine specified!";
		throw new DTC_DataCorruptionException();
	}

	if (info->currentReadPtr != nullptr) {
		TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEFORE BUFFER CHECK info->currentReadPtr="
									 << (void*)info->currentReadPtr << " *nextReadPtr_=0x" << std::hex
									 << *(uint16_t*)info->currentReadPtr;
	}
	else
	{
		TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket BEFORE BUFFER CHECK info->currentReadPtr=nullptr";
	}

	auto index = GetCurrentBuffer(info);

	// Need new buffer if GetCurrentBuffer returns -1 (no buffers) or -2 (done with all held buffers)
	if (index < 0) {
		TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket Obtaining new " << (engine == DTC_DMA_Engine_DAQ ? "DAQ" : "DCS")
									 << " Buffer";

		void* oldBufferPtr = nullptr;
		if (info->buffer.size() > 0) oldBufferPtr = &info->buffer.back()[0];
		auto sts = ReadBuffer(engine, tmo_ms);  // does return code
		if (sts <= 0) {
			TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: ReadBuffer returned " << sts << ", returning nullptr";
			return nullptr;
		}
		// MUST BE ABLE TO HANDLE daqbuffer_==nullptr OR retry forever?
		info->currentReadPtr = &info->buffer.back()[0];
		TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket info->currentReadPtr=" << (void*)info->currentReadPtr
									 << " *info->currentReadPtr=0x" << std::hex << *(unsigned*)info->currentReadPtr
									 << " lastReadPtr_=" << (void*)info->lastReadPtr;
		void* bufferIndexPointer = static_cast<uint8_t*>(info->currentReadPtr) + 2;
		if (info->currentReadPtr == oldBufferPtr && info->bufferIndex == *static_cast<uint32_t*>(bufferIndexPointer)) {
			TLOG(TLVL_ReadNextDAQPacket)
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
	TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket reading next packet from buffer: info->currentReadPtr="
								 << (void*)info->currentReadPtr;

	auto blockByteCount = *reinterpret_cast<uint16_t*>(info->currentReadPtr);
	TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: blockByteCount=" << blockByteCount
								 << ", info->currentReadPtr=" << (void*)info->currentReadPtr
								 << ", *nextReadPtr=" << (int)*((uint16_t*)info->currentReadPtr);
	if (blockByteCount == 0 || blockByteCount == 0xcafe) {
		if (static_cast<size_t>(index) < info->buffer.size() - 1) {
			TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: blockByteCount is invalid, moving to next buffer";
			auto nextBufferPtr = *info->buffer[index + 1];
			info->currentReadPtr = nextBufferPtr + 8;  // Offset past DMA header
			return ReadNextPacket(engine, tmo_ms);     // Recursion
		}
		else
		{
			TLOG(TLVL_ReadNextDAQPacket)
				<< "ReadNextPacket: blockByteCount is invalid, and this is the last buffer! Returning nullptr!";
			info->currentReadPtr = nullptr;
			// This buffer is invalid, release it!
			// Try and see if we're merely stuck...hopefully, all the data is out of the buffers...
			device_.read_release(engine, 1);
			return nullptr;
		}
	}

	auto test = std::make_unique<DTC_DataPacket>(info->currentReadPtr);
	TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: current+blockByteCount="
								 << (void*)(reinterpret_cast<uint8_t*>(info->currentReadPtr) + blockByteCount)
								 << ", end of dma buffer="
								 << (void*)(info->buffer[index][0] + GetBufferByteCount(info, index) +
											8);  // +8 because first 8 bytes are not included in byte count
	if (reinterpret_cast<uint8_t*>(info->currentReadPtr) + blockByteCount >
		info->buffer[index][0] + GetBufferByteCount(info, index) + 8) {
		blockByteCount = static_cast<uint16_t>(
			info->buffer[index][0] + GetBufferByteCount(info, index) + 8 -
			reinterpret_cast<uint8_t*>(info->currentReadPtr));  // +8 because first 8 bytes are not included in byte count
		TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: Adjusting blockByteCount to " << blockByteCount
									 << " due to end-of-DMA condition";
		test->SetWord(0, blockByteCount & 0xFF);
		test->SetWord(1, (blockByteCount >> 8));
	}

	TLOG(TLVL_ReadNextDAQPacket) << test->toJSON();

	// Update the packet pointers

	// lastReadPtr_ is easy...
	info->lastReadPtr = info->currentReadPtr;

	// Increment by the size of the data block
	info->currentReadPtr = reinterpret_cast<char*>(info->currentReadPtr) + blockByteCount;

	TLOG(TLVL_ReadNextDAQPacket) << "ReadNextPacket: RETURN";
	return test;
}

void DTCLib::DTC::WriteDetectorEmulatorData(mu2e_databuff_t* buf, size_t sz)
{
	if (sz < dmaSize_) {
		sz = dmaSize_;
	}
	auto retry = 3;
	int errorCode;
	do
	{
		TLOG(TLVL_WriteDetectorEmulatorData) << "WriteDetectorEmulatorData: Writing buffer of size " << sz;
		errorCode = device_.write_data(DTC_DMA_Engine_DAQ, buf, sz);
		retry--;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0) {
		TLOG(TLVL_ERROR) << "WriteDetectorEmulatorData: write_data returned " << errorCode
						 << ", throwing DTC_IOErrorException!";
		throw DTC_IOErrorException(errorCode);
	}
}

//
// Private Functions.
//
int DTCLib::DTC::ReadBuffer(const DTC_DMA_Engine& channel, int tmo_ms)
{
	mu2e_databuff_t* buffer;
	auto retry = 2;
	int errorCode;
	do
	{
		TLOG(TLVL_ReadBuffer) << "ReadBuffer before device_.read_data";
		errorCode = device_.read_data(channel, reinterpret_cast<void**>(&buffer), tmo_ms);
		retry--;
		// if (errorCode == 0) usleep(1000);
	} while (retry > 0 && errorCode == 0);
	if (errorCode == 0) {
		TLOG(TLVL_ReadBuffer) << "ReadBuffer: Device timeout occurred! ec=" << errorCode << ", rt=" << retry;
	}
	else if (errorCode < 0)
	{
		TLOG(TLVL_ERROR) << "ReadBuffer: read_data returned " << errorCode << ", throwing DTC_IOErrorException!";
		throw DTC_IOErrorException(errorCode);
	}
	else
	{
		TLOG(TLVL_ReadBuffer) << "ReadBuffer buffer_=" << (void*)buffer << " errorCode=" << errorCode << " *buffer_=0x"
							  << std::hex << *(unsigned*)buffer;
		if (channel == DTC_DMA_Engine_DAQ) {
			daqDMAInfo_.buffer.push_back(buffer);
			TLOG(TLVL_ReadBuffer) << "ReadBuffer: There are now " << daqDMAInfo_.buffer.size()
								  << " DAQ buffers held in the DTC Library";
		}
		else if (channel == DTC_DMA_Engine_DCS)
		{
			dcsDMAInfo_.buffer.push_back(buffer);
			TLOG(TLVL_ReadBuffer) << "ReadBuffer: There are now " << dcsDMAInfo_.buffer.size()
								  << " DCS buffers held in the DTC Library";
		}
	}
	return errorCode;
}

void DTCLib::DTC::ReleaseBuffers(const DTC_DMA_Engine& channel)
{
	TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers BEGIN";
	DMAInfo* info;
	if (channel == DTC_DMA_Engine_DAQ)
		info = &daqDMAInfo_;
	else if (channel == DTC_DMA_Engine_DCS)
		info = &dcsDMAInfo_;
	else
	{
		TLOG(TLVL_ERROR) << "ReadNextPacket: Invalid DMA Engine specified!";
		throw new DTC_DataCorruptionException();
	}

	auto releaseBufferCount = GetCurrentBuffer(info);
	if (releaseBufferCount > 0) {
		TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers releasing " << releaseBufferCount << " "
								  << (channel == DTC_DMA_Engine_DAQ ? "DAQ" : "DCS") << " buffers.";
		device_.read_release(channel, releaseBufferCount);

		for (int ii = 0; ii < releaseBufferCount; ++ii) {
			info->buffer.pop_front();
		}
	}
	else
	{
		TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers releasing ALL " << (channel == DTC_DMA_Engine_DAQ ? "DAQ" : "DCS")
								  << " buffers.";
		ReleaseAllBuffers(channel);
	}
	TLOG(TLVL_ReleaseBuffers) << "ReleaseBuffers END";
}

int DTCLib::DTC::GetCurrentBuffer(DMAInfo* info)
{
	TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer BEGIN";
	if (info->currentReadPtr == nullptr || info->buffer.size() == 0) {
		TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer returning -1 because not currently reading a buffer";
		return -1;
	}

	for (size_t ii = 0; ii < info->buffer.size(); ++ii) {
		auto bufferptr = *info->buffer[ii];
		uint16_t bufferSize = *reinterpret_cast<uint16_t*>(bufferptr);
		if (info->currentReadPtr > bufferptr &&
			info->currentReadPtr < bufferptr + bufferSize + 8) {  // +8 because first 8 bytes are not included in byte count
			TLOG(TLVL_GetCurrentBuffer) << "Found matching buffer at index " << ii << ".";
			return ii;
		}
	}
	TLOG(TLVL_GetCurrentBuffer) << "GetCurrentBuffer returning -2: Have buffers but none match, need new";
	return -2;
}

uint16_t DTCLib::DTC::GetBufferByteCount(DMAInfo* info, size_t index)
{
	if (index >= info->buffer.size()) return 0;
	auto bufferptr = *info->buffer[index];
	uint16_t bufferSize = *reinterpret_cast<uint16_t*>(bufferptr);
	return bufferSize;
}

void DTCLib::DTC::WriteDataPacket(const DTC_DataPacket& packet)
{
	TLOG(TLVL_WriteDataPacket) << "WriteDataPacket: Writing packet: " << packet.toJSON();
	mu2e_databuff_t buf;
	uint64_t size = packet.GetSize() + sizeof(uint64_t);
	//	uint64_t packetSize = packet.GetSize();
	if (size < static_cast<uint64_t>(dmaSize_)) size = dmaSize_;

	memcpy(&buf[0], &size, sizeof(uint64_t));
	memcpy(&buf[8], packet.GetData(), packet.GetSize() * sizeof(uint8_t));

	{
		std::stringstream ss;
		ss << std::setfill('0') << std::hex;
		for (uint16_t ii = 0; ii < size - 1; ii += 2) {
			ss << "0x" << std::setw(2) << static_cast<int>(buf[ii + 1]) << " ";
			ss << "" << std::setw(2) << static_cast<int>(buf[ii]) << "\n";
		}
		ss << std::dec;

		std::cout << "Buffer being sent: \n" << ss.str() << std::endl;
	}

	auto retry = 3;
	int errorCode;
	do
	{
		TLOG(TLVL_WriteDataPacket) << "Attempting to write data...";
		errorCode = device_.write_data(DTC_DMA_Engine_DCS, &buf, size);
		TLOG(TLVL_WriteDataPacket) << "Attempted to write data, errorCode=" << errorCode << ", retries=" << retry;
		retry--;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0) {
		TLOG(TLVL_ERROR) << "WriteDataPacket: write_data returned " << errorCode << ", throwing DTC_IOErrorException!";
		throw DTC_IOErrorException(errorCode);
	}
}

void DTCLib::DTC::WriteDMAPacket(const DTC_DMAPacket& packet)
{
	WriteDataPacket(packet.ConvertToDataPacket());
}
