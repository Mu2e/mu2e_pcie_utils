// This file (util_main.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 13, 2015. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <vector>
#include <fstream>
#include <unordered_map>
#include <bitset>

#include "mu2e_driver/mu2e_mmap_ioctl.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "data_file_verifier"

#define MU2E_DATA_FORMAT_VERSION 6

typedef std::unordered_map<uint8_t, std::bitset<6>> dtc_map;
typedef std::unordered_map<uint8_t, dtc_map> subsystem_map;
typedef std::unordered_map<uint64_t, subsystem_map> event_map;

std::string getLongOptionOption(int* index, char** argv[])
{
	auto arg = std::string((*argv)[*index]);
	auto pos = arg.find('=');

	if (pos == std::string::npos)
	{
		return arg;
	}
	else
	{
		return arg.substr(0, pos - 1);
	}
}

void printHelpMsg()
{
	std::cout << "Verifies the DMA and Data block content of DTC binary file(s)" << std::endl;
	std::cout << "Usage: data_file_verifier [options] file.."
			  << std::endl;
	std::cout
		<< "Options are:" << std::endl
		<< "    -h, --help: This message." << std::endl
		<< std::endl;
	exit(0);
}

bool VerifyTrackerDataBlock(DataHeaderPacket* blockPtr)
{
	auto blockByteSize = blockPtr->s.TransferByteCount;
	if (blockByteSize != 0x10 && blockByteSize != 0x30)
	{
		TLOG(TLVL_ERROR) << "VerifyTrackerDataBlock: Block has incorrect size 0x" << std::hex << blockByteSize << ". Valid sizes are 0x10 and 0x30";
		return false;
	}

	if (blockByteSize == 0x30)
	{
		auto firstDataPacket = *reinterpret_cast<DataPacket*>(blockPtr + 1);
		auto secondDataPacket = *reinterpret_cast<DataPacket*>(blockPtr + 2);

		auto strawIndex = firstDataPacket.data10;
		if (strawIndex > 23039)
		{
			TLOG(TLVL_ERROR) << "VerifyTrackerDataBlock: strawIndex " << strawIndex << " is out-of-range! (Max 23039)";
			return false;
		}
		if ((secondDataPacket.dataFE & 0xF0) != 0)
		{
			TLOG(TLVL_ERROR) << "VerifyTrackerDataBlock: Data present in Reserved word: 0x" << std::hex << ((secondDataPacket.dataFE & 0xF0) >> 4);
			return false;
		}
	}

	return true;
}

bool VerifyCalorimeterDataBlock(DataHeaderPacket* blockPtr)
{
	auto blockByteSize = blockPtr->s.TransferByteCount;

	if (blockByteSize == 0x10)
	{
		//TLOG(TLVL_WARNING) << "VerifyCalorimeterDataBlock: Empty block encountered!";
		return true;
	}

	auto dataPtr = reinterpret_cast<uint16_t*>(blockPtr + 1);
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

	if (channelStatusA != 0x3F || channelStatusB != 0x3FFF)
	{
		TLOG(TLVL_WARNING) << "VerifyCalorimeterDataBlock: Not all 20 channels are enabled! StsA: 0x" << std::hex << channelStatusA << ", stsB: 0x" << std::hex << channelStatusB;
		// Not sure if this is a fatal error or not, leaving it for now
		// return false;
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

		dataPtr += 4;
		currentOffset += 8;

		auto numSamples = *dataPtr & 0xFF;
		auto maxSample = (*dataPtr & 0xFF00) >> 8;
		auto currentMaximumValue = 0;
		auto currentMaximumIndex = 0;

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

bool VerifyCRVDataBlock(DataHeaderPacket* blockPtr)
{
	TLOG(TLVL_WARNING) << "VerifyCRVDataBlock: Not yet implemented!";
	return true;
}

int main(int argc, char* argv[])
{
	std::vector<std::string> binaryFiles;
	for (auto optind = 1; optind < argc; ++optind)
	{
		if (argv[optind][0] == '-')
		{
			switch (argv[optind][1])
			{
				case '-':  // Long option
				{
					auto option = getLongOptionOption(&optind, &argv);
					if (option == "--help")
					{
						printHelpMsg();
					}
					break;
				}
				default:
					TLOG(TLVL_ERROR) << "Unknown option: " << argv[optind] << std::endl;
					printHelpMsg();
					break;
				case 'h':
					printHelpMsg();
					break;
			}
		}
		else
		{
			binaryFiles.push_back(std::string(argv[optind]));
		}
	}

	for (auto& file : binaryFiles)
	{
		event_map events;
		bool success = true;
		std::ifstream is(file);
		if (is.bad() || !is)
		{
			TLOG(TLVL_ERROR) << "Cannot read file " << file;
			continue;
		}
		TLOG(TLVL_INFO) << "Reading binary file " << file;
		uint64_t total_size_read = 0;

		mu2e_databuff_t buf;

		bool continueFile = true;
		while (is && continueFile)
		{
			uint64_t dmaWriteSize;  // DMA Write buffer size
			is.read((char*)&dmaWriteSize, sizeof(dmaWriteSize));
			total_size_read += sizeof(dmaWriteSize);

			uint64_t dmaSize;  // DMA buffer size
			is.read((char*)&dmaSize, sizeof(dmaSize));
			total_size_read += sizeof(dmaSize);

			// Check that DMA Write Buffer Size = DMA Buffer Size + 16
			if (dmaSize + 16 != dmaWriteSize)
			{
				TLOG(TLVL_ERROR) << "Buffer error detected: DMA Size mismatch at 0x" << std::hex << total_size_read << ". Write size: " << dmaWriteSize << ", DMA Size: " << dmaSize;
				break;
			}

			// Check that size of all DataBlocks = DMA Buffer Size
			is.read((char*)buf, dmaSize);
			total_size_read += dmaSize;

			size_t offset = 0;
			while (offset < dmaSize)
			{
				auto header = *reinterpret_cast<DataHeaderPacket*>(buf + offset);
				auto blockByteSize = header.s.TransferByteCount;

				// Check that this is indeed a DataHeader packet
				auto dataHeaderMask = 0x80F0;
				auto dataHeaderTest = header.w.w1;
				TLOG(TLVL_TRACE) << "Block size: 0x" << std::hex << blockByteSize << ", Test word: " << std::hex << dataHeaderTest << ", masked: " << (dataHeaderTest & dataHeaderMask) << " =?= 0x8050";
				if ((dataHeaderTest & dataHeaderMask) != 0x8050)
				{
					TLOG(TLVL_ERROR) << "Encountered bad data at 0x" << std::hex << (total_size_read - dmaSize + offset) << ": expected DataHeader, got 0x" << std::hex << *reinterpret_cast<uint64_t*>(buf + offset);
					TLOG(TLVL_ERROR) << "This most likely means that the declared DMA size is incorrect, it was declared as 0x" << std::hex << dmaSize << ", but we ran out of DataHeaders at 0x" << std::hex << offset;
					// go to next file
					continueFile = false;
					break;
				}
				if (offset + blockByteSize > dmaSize)
				{
					TLOG(TLVL_ERROR) << "Block goes past end of DMA! Blocks should always end at DMA boundary! Error at 0x" << std::hex << (total_size_read - dmaSize + offset);
					// go to next file
					continueFile = false;
					break;
				}

				auto packetCountTest = header.s.PacketCount;
				if ((packetCountTest + 1) * 16 != blockByteSize)
				{
					TLOG(TLVL_ERROR) << "Block data packet count and byte count disagree! packetCount: " << packetCountTest << ", which implies block size of 0x" << std::hex << ((packetCountTest + 1) * 16) << ", blockSize: " << std::hex << blockByteSize;
					success = false;

					// We don't have to skip to the next file, because we already know the data block integrity is fine.
					offset += blockByteSize;
					continue;
				}

				auto dataVersionTest = header.s.Version;
				if (MU2E_DATA_FORMAT_VERSION != dataVersionTest)
				{
					TLOG(TLVL_INFO) << "Data format version for binary file " << dataVersionTest << " does not match expected version " << MU2E_DATA_FORMAT_VERSION << ". Not performing subsystem-level validation";
					success = false;

					// We don't have to skip to the next file, because we already know the data block integrity is fine.
					offset += blockByteSize;
					continue;
				}

				uint64_t timestamp = header.s.ts10 + (static_cast<uint64_t>(header.s.ts32) << 16) + (static_cast<uint64_t>(header.s.ts54) << 32);
				if (events.count(timestamp) && events[timestamp].count(header.s.SubsystemID) && events[timestamp][header.s.SubsystemID].count(header.s.DTCID) && events[timestamp][header.s.SubsystemID][header.s.DTCID][header.s.LinkID])
				{
					TLOG(TLVL_ERROR) << "Duplicate DataBlock detected for TS " << timestamp << " SS " << header.s.SubsystemID << " DTC " << header.s.DTCID << " ROC " << header.s.LinkID << "!";
					success = false;
				}
				else
				{
					events[timestamp][header.s.SubsystemID][header.s.DTCID][header.s.LinkID] = true;
				}

				auto subsystemID = header.s.SubsystemID;
				bool subsystemCheck = true;
				switch (subsystemID)
				{
					case 0:  // Tracker
						subsystemCheck = VerifyTrackerDataBlock(reinterpret_cast<DataHeaderPacket*>(buf + offset));
						break;
					case 1:  // Calorimeter
						subsystemCheck = VerifyCalorimeterDataBlock(reinterpret_cast<DataHeaderPacket*>(buf + offset));
						break;
					case 2:  // CRV
						subsystemCheck = VerifyCRVDataBlock(reinterpret_cast<DataHeaderPacket*>(buf + offset));
						break;
					default:
						TLOG(TLVL_INFO) << "Data-level verification not implemented for subsystem ID " << subsystemID;
						break;
				}
				if (!subsystemCheck)
				{
					TLOG(TLVL_ERROR) << "Data block at 0x" << std::hex << (total_size_read - dmaSize + offset) << " is not a valid data block for subsystem ID " << subsystemID;
					success = false;
					// We don't have to skip to the next file, because we already know the data block integrity is fine.
				}
				offset += blockByteSize;
			}
		}

		if (success)
		{
			TLOG(TLVL_INFO) << "File " << file << " verified successfully!";
		}
		else
		{
			TLOG(TLVL_WARNING) << "File " << file << " had verification errors, see TRACE output for details";
		}
	}
}