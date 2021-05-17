
#include <vector>
#include <fstream>
#include <unordered_map>
#include <bitset>

#include "dtcInterfaceLib/DTC_Packets.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"

namespace DTCLib {
class DTC_Data_Verifier
{
public:
	bool VerifyTrackerDataBlock(DTCLib::DTC_DataBlock /*block*/)
	{
		return true;
	}

	bool VerifyCalorimeterDataBlock(DTCLib::DTC_DataBlock block)
	{
		auto blockByteSize = block.GetHeader().GetByteCount();

		if (blockByteSize == 0x10)
		{
			//TLOG(TLVL_WARNING) << "VerifyCalorimeterDataBlock: Empty block encountered!";
			return true;
		}

		auto dataPtr = reinterpret_cast<const uint16_t*>(block.GetData());
		auto hitCount = *dataPtr;
		auto currentOffset = 0;

		std::vector<uint16_t> hitOffsets;
		for (int ii = 0; ii < hitCount; ++ii)
		{
			hitOffsets.push_back(*(++dataPtr));
			currentOffset += 2;
			if (currentOffset > blockByteSize)
			{
				TLOG(TLVL_ERROR) << "VerifyCalorimeterDataBlock: Calorimeter data extends past declared block size! (0x" << std::hex << currentOffset << " > 0x" << std::hex << blockByteSize << ")";
				return false;
			}
		}

		++dataPtr;
		currentOffset += 2;

		auto channelStatusA = (*dataPtr & 0xFC00) >> 10;

		++dataPtr;
		currentOffset += 2;

		auto channelStatusB = (*dataPtr & 0x3FFF);
		if ((*dataPtr & 0xC000) != 0)
		{
			TLOG(TLVL_ERROR) << "VerifyCalorimeterDataBlock: Data present in BoardID Reserved field!";
			return false;
		}

		if (channelStatusA == 0x0 && channelStatusB != 0x0)
		{
			TLOG(TLVL_WARNING) << "VerifyCalorimeterDataBlock: None of the 20 channels are enabled! StsA: 0x" << std::hex << channelStatusA << ", stsB: 0x" << std::hex << channelStatusB;
			// Not sure if this is a fatal error or not, leaving it for now
			// return false;
		}

		if (hitCount == 0) {
			TLOG(TLVL_WARNING) << "VerifyCalorimeterDataBlock: There are zero hits in this block!";
		}

		for (int ii = 0; ii < hitCount; ++ii)
		{
			++dataPtr;
			currentOffset += 2;
			if (currentOffset != hitOffsets[ii])
			{
				TLOG(TLVL_ERROR) << "VerifyCalorimeterDataBlock: Hit " << ii << " index value " << hitOffsets[ii] << " does not agree with current offset " << currentOffset;
				return false;
			}


			//auto channelNumber = *dataPtr & 0x3F;
			//auto diracA = (*dataPtr >> 6) & 0x3FF;
			++dataPtr;
			auto diracB = *dataPtr;

			auto sipmID = diracB >> 12;
			auto crystalID = diracB & 0xFFF;

			if(sipmID != 0 && sipmID != 1){ TLOG(TLVL_WARNING) << "Invalid sipmID " << sipmID << " detected!"; }
			if(crystalID > 674 * 2) { TLOG(TLVL_WARNING) << "Invalid crystalID " << crystalID << " detected!"; } 

			dataPtr += 2;

			auto time = *dataPtr;
			if(time < 500) { TLOG(TLVL_WARNING) << "VerifyCalorimeterBlock: Suspicious time " << time << " detected!";}

			++dataPtr;
			currentOffset += 8;

			auto numSamples = *dataPtr & 0xFF;
			auto maxSample = (*dataPtr & 0xFF00) >> 8;
			auto currentMaximumValue = 0;
			auto currentMaximumIndex = 0;

			if (numSamples == 0) {
				TLOG(TLVL_WARNING) << "VerifyCalorimeterBlock: This hit has zero samples!";
			}

			for (int jj = 0; jj < numSamples; ++jj)
			{
				++dataPtr;
				currentOffset += 2;

				if (*dataPtr > currentMaximumValue)
				{
					currentMaximumValue = *dataPtr;
					currentMaximumIndex = jj;
				}
			}

			if (maxSample != currentMaximumIndex)
			{
				TLOG(TLVL_ERROR) << "VerifyCalorimeterDataBlock: Hit " << ii << " has mismatched maximum sample; expected " << maxSample << ", actual maximum " << currentMaximumIndex;
				return false;
			}
		}

		++dataPtr;
		currentOffset += 2;
		while (currentOffset % 16 != 0)
		{
			if (*dataPtr != 0)
			{
				TLOG(TLVL_ERROR) << "VerifyCalorimeterDataBlock: Data detected in end padding: 0x" << std::hex << *dataPtr;
				return false;
			}
			++dataPtr;
			currentOffset += 2;
		}

		return true;
	}

	bool VerifyCRVDataBlock(DTCLib::DTC_DataBlock /*block*/)
	{
		return true;
	}

	bool VerifyROCEmulatorBlock(DTCLib::DTC_DataBlock block)
	{
		auto headerDP = block.GetHeader().ConvertToDataPacket();

		bool success = true;
		success = headerDP.GetWord(12) == 0xEF;
		success &= headerDP.GetWord(13) == 0xBE;
		success &= headerDP.GetWord(15) == 0xBE;
		if (!success) {
			TLOG(TLVL_ERROR) << "VerifyROCEmulatorBlock: Header format is incorrect (check bytes)";
			return false;
		}

		auto packetCount = block.GetHeader().GetPacketCount();
		auto dataPtr = reinterpret_cast<uint16_t const*>(block.GetData());
		
		for (int ii = 0; ii < packetCount; ++ii) 
		{
			success = *dataPtr == 0x2222;
			++dataPtr;
			success &= *dataPtr == 0x1111;
			++dataPtr;

			uint32_t roc_packet_count_test = *dataPtr;
			++dataPtr;
			roc_packet_count_test += (*dataPtr << 16);
			++dataPtr;
			if (roc_packet_count_test != roc_emulator_packet_count_)
			{
				TLOG(TLVL_INFO) << "VerifyROCEmulatorBlock: ROC Emulator packet count unexpected, shifting from " << roc_emulator_packet_count_ << " to " << roc_packet_count_test;
				roc_emulator_packet_count_ = roc_packet_count_test;
			}

			success &= *dataPtr == 0x4444;
			++dataPtr;
			success &= *dataPtr == 0x3333;
			++dataPtr;

			roc_packet_count_test = *dataPtr;
			++dataPtr;
			roc_packet_count_test += (*dataPtr << 16);
			success &= roc_packet_count_test == roc_emulator_packet_count_;
			++dataPtr;

			roc_emulator_packet_count_++;

			if (!success) {
				TLOG(TLVL_ERROR) << "VerifyROCEmulatorBlock: Data packet " << ii << " has format error";
			}
		}

		return true;
	}

	bool VerifyBlock(DTCLib::DTC_DataBlock block, uint64_t dmaSize)
	{
		auto header = block.GetHeader();
		auto blockByteSize = header.GetByteCount();

		// Check that this is indeed a DataHeader packet
		auto dataHeaderMask = 0x80F0;
		uint16_t dataHeaderTest = static_cast<uint16_t>(header.ConvertToDataPacket().GetWord(2)) + (static_cast<uint16_t>(header.ConvertToDataPacket().GetWord(3)) << 8);
		TLOG(TLVL_DEBUG + 5) << "Block size: 0x" << std::hex << blockByteSize << ", Test word: " << std::hex << dataHeaderTest << ", masked: " << (dataHeaderTest & dataHeaderMask) << " =?= 0x8050";
		if ((dataHeaderTest & dataHeaderMask) != 0x8050)
		{
			TLOG(TLVL_ERROR) << "Encountered bad data at 0x" << std::hex << (total_size_read_ - dmaSize + current_buffer_pos_) << ": expected DataHeader, got 0x" << std::hex << *reinterpret_cast<const uint64_t*>(block.GetData());
			TLOG(TLVL_ERROR) << "This most likely means that the declared DMA size is incorrect, it was declared as 0x" << std::hex << dmaSize << ", but we ran out of DataHeaders at 0x" << std::hex << current_buffer_pos_;
			// go to next file
			continueFile_ = false;
			return false;
		}
		if (current_buffer_pos_ + blockByteSize > dmaSize)
		{
			TLOG(TLVL_ERROR) << "Block goes past end of DMA! Blocks should always end at DMA boundary! Error at 0x" << std::hex << (total_size_read_ - dmaSize + current_buffer_pos_);
			// go to next file
			continueFile_ = false;
			return false;
		}

		auto packetCountTest = header.GetPacketCount();
		if ((packetCountTest + 1) * 16 != blockByteSize)
		{
			TLOG(TLVL_ERROR) << "Block data packet count and byte count disagree! packetCount: " << packetCountTest << ", which implies block size of 0x" << std::hex << ((packetCountTest + 1) * 16) << ", blockSize: 0x" << std::hex << blockByteSize;

			// We don't have to skip to the next file, because we already know the data block integrity is fine.
			current_buffer_pos_ += blockByteSize;
			return false;
		}

		auto subsystemID = header.GetSubsystemID();
		bool subsystemCheck = true;
		switch (subsystemID)
		{
			case 0:  // Tracker
				subsystemCheck = VerifyTrackerDataBlock(block);
				break;
			case 1:  // Calorimeter
				subsystemCheck = VerifyCalorimeterDataBlock(block);
				break;
			case 2:  // CRV
				subsystemCheck = VerifyCRVDataBlock(block);
				break;
			case 3: // ROC Emulator
				subsystemCheck = VerifyROCEmulatorBlock(block);
				break;
			default:
				TLOG(TLVL_INFO) << "Data-level verification not implemented for subsystem ID " << subsystemID;
				break;
		}
		if (!subsystemCheck)
		{
			TLOG(TLVL_ERROR) << "Data block at 0x" << std::hex << (total_size_read_ - dmaSize + current_buffer_pos_) << " is not a valid data block for subsystem ID " << static_cast<int>(subsystemID);
			current_buffer_pos_ += blockByteSize;

			return false;
			// We don't have to skip to the next file, because we already know the data block integrity is fine.
		}

		current_buffer_pos_ += blockByteSize;
		return true;
	}

	bool VerifySubEvent(DTCLib::DTC_SubEvent subevt, DTCLib::DTC_EventWindowTag eventTag, uint64_t dmaSize)
	{
		if (subevt.GetEventWindowTag() != eventTag)
		{
			TLOG(TLVL_WARNING) << "Event Window Tag from Event does not agree with EWT from SubEvent! (" << eventTag.GetEventWindowTag(true) << " != " << subevt.GetEventWindowTag().GetEventWindowTag(true) << ")";
		}
		if (subevt.GetHeader()->num_rocs != subevt.GetDataBlockCount())
		{
			TLOG(TLVL_WARNING) << "SubEvent Header num_rocs field disagrees with number of DataBlocks! (" << subevt.GetHeader()->num_rocs << " != " << subevt.GetDataBlockCount() << ")";
		}
		TLOG(TLVL_DEBUG + 4) << subevt.GetHeader()->toJson();
		current_buffer_pos_ += sizeof(DTCLib::DTC_SubEventHeader);

		bool success = true;
		for (auto& block : subevt.GetDataBlocks())
		{
			auto blockSuccess = VerifyBlock(block, dmaSize);
			success &= blockSuccess;
			if (!continueFile_) return false;
		}
		return success;
	}

	bool VerifyEvent(DTCLib::DTC_Event evt, uint64_t dmaSize)
	{
		bool success = true;

		// Check if the DMA size is inclusive or not
		if (evt.GetEventByteCount() != dmaSize && evt.GetEventByteCount() != dmaSize - 8 && dmaSize != 0)
		{
			TLOG(TLVL_ERROR) << "Event Header byte count (" << evt.GetEventByteCount() << ") does not match DMA byte count (" << dmaSize << ")!";
			return false;
		}

		if (evt.GetHeader()->num_dtcs != evt.GetSubEventCount())
		{
			TLOG(TLVL_WARNING) << "Event Header num_dtcs field disagrees with number of SubEvents! (" << evt.GetHeader()->num_dtcs << " != " << evt.GetSubEventCount() << ")";
		}

		TLOG(TLVL_DEBUG + 3) << evt.GetHeader()->toJson();
		auto eventTag = evt.GetEventWindowTag();

		current_buffer_pos_ += sizeof(DTCLib::DTC_EventHeader);

		for (auto& subevt : evt.GetSubEvents())
		{
			auto subevtSuccess = VerifySubEvent(subevt, eventTag, dmaSize);
			success &= subevtSuccess;
			if (!continueFile_) return false;
		}
		return success;
	}

	bool VerifyFile(std::string file)
	{
		std::ifstream is(file);
		if (is.bad() || !is)
		{
			TLOG(TLVL_ERROR) << "Cannot read file " << file;
			return false;
		}
		TLOG(TLVL_INFO) << "Reading binary file " << file;
		total_size_read_ = 0;

		mu2e_databuff_t buf;

		bool success = true;
		continueFile_ = true;
		while (is && continueFile_)
		{
			uint64_t dmaWriteSize;  // DMA Write buffer size
			is.read((char*)&dmaWriteSize, sizeof(dmaWriteSize));
			total_size_read_ += sizeof(dmaWriteSize);

			uint64_t dmaSize;
			is.read((char*)&dmaSize, sizeof(dmaSize));
			total_size_read_ += sizeof(dmaSize);

			// Check that DMA Write Buffer Size = DMA Buffer Size + 16
			if (dmaSize + 16 != dmaWriteSize)
			{
				TLOG(TLVL_ERROR) << "Buffer error detected: DMA Size mismatch at 0x" << std::hex << total_size_read_ << ". Write size: " << dmaWriteSize << ", DMA Size: " << dmaSize;
				success = false;
				break;
			}

			// Check that size of all DataBlocks = DMA Buffer Size
			is.read((char*)buf, dmaSize);
			total_size_read_ += dmaSize;

			current_buffer_pos_ = 0;

			TLOG(TLVL_DEBUG + 1) << "Verifying event at offset 0x" << std::hex << (total_size_read_ - dmaSize);
			DTCLib::DTC_Event thisEvent(buf);
			success = VerifyEvent(thisEvent, dmaSize);
		}

		if (success)
		{
			TLOG(TLVL_INFO) << "File " << file << " verified successfully!";
		}
		else
		{
			TLOG(TLVL_WARNING) << "File " << file << " had verification errors, see TRACE output for details";
		}

		return success;
	}

private:
	bool continueFile_{true};
	uint64_t total_size_read_{0};
	uint64_t current_buffer_pos_{0};
	uint32_t roc_emulator_packet_count_{0};
};
}  // namespace DTCLib
