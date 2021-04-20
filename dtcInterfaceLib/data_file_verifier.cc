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

#include "dtcInterfaceLib/DTC_Packets.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "data_file_verifier"

#define MU2E_DATA_FORMAT_VERSION 6

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

bool continueFile = true;
uint64_t total_size_read = 0;
uint64_t current_buffer_pos = 0;
uint64_t dmaSize = 0;

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

bool VerifyCRVDataBlock(DTCLib::DTC_DataBlock /*block*/)
{
	return true;
}

bool VerifyBlock(DTCLib::DTC_DataBlock block)
{
	auto header = block.GetHeader();
	auto blockByteSize = header.GetByteCount();

	// Check that this is indeed a DataHeader packet
	auto dataHeaderMask = 0x80F0;
	uint16_t dataHeaderTest = static_cast<uint16_t>(header.ConvertToDataPacket().GetWord(2)) + (static_cast<uint16_t>(header.ConvertToDataPacket().GetWord(3)) << 8);
	TLOG(TLVL_DEBUG + 5) << "Block size: 0x" << std::hex << blockByteSize << ", Test word: " << std::hex << dataHeaderTest << ", masked: " << (dataHeaderTest & dataHeaderMask) << " =?= 0x8050";
	if ((dataHeaderTest & dataHeaderMask) != 0x8050)
	{
		TLOG(TLVL_ERROR) << "Encountered bad data at 0x" << std::hex << (total_size_read - dmaSize + current_buffer_pos) << ": expected DataHeader, got 0x" << std::hex << *reinterpret_cast<const uint64_t*>(block.GetData());
		TLOG(TLVL_ERROR) << "This most likely means that the declared DMA size is incorrect, it was declared as 0x" << std::hex << dmaSize << ", but we ran out of DataHeaders at 0x" << std::hex << current_buffer_pos;
		// go to next file
		continueFile = false;
		return false;
	}
	if (current_buffer_pos + blockByteSize > dmaSize)
	{
		TLOG(TLVL_ERROR) << "Block goes past end of DMA! Blocks should always end at DMA boundary! Error at 0x" << std::hex << (total_size_read - dmaSize + current_buffer_pos);
		// go to next file
		continueFile = false;
		return false;
	}

	auto packetCountTest = header.GetPacketCount();
	if ((packetCountTest + 1) * 16 != blockByteSize)
	{
		TLOG(TLVL_ERROR) << "Block data packet count and byte count disagree! packetCount: " << packetCountTest << ", which implies block size of 0x" << std::hex << ((packetCountTest + 1) * 16) << ", blockSize: 0x" << std::hex << blockByteSize;

		// We don't have to skip to the next file, because we already know the data block integrity is fine.
		current_buffer_pos += blockByteSize;
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
		default:
			TLOG(TLVL_INFO) << "Data-level verification not implemented for subsystem ID " << subsystemID;
			break;
	}
	if (!subsystemCheck)
	{
		TLOG(TLVL_ERROR) << "Data block at 0x" << std::hex << (total_size_read - dmaSize + current_buffer_pos) << " is not a valid data block for subsystem ID " << subsystemID;
		current_buffer_pos += blockByteSize;

		return false;
		// We don't have to skip to the next file, because we already know the data block integrity is fine.
	}

	current_buffer_pos += blockByteSize;
	return true;
}

bool VerifySubEvent(DTCLib::DTC_SubEvent subevt, DTCLib::DTC_EventWindowTag eventTag)
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
	current_buffer_pos += sizeof(DTCLib::DTC_SubEventHeader);

	bool success = true;
	for (auto& block : subevt.GetDataBlocks())
	{
		auto blockSuccess = VerifyBlock(block);
		success &= blockSuccess;
		if (!continueFile) return false;
	}
	return success;
}

bool VerifyEvent(DTCLib::DTC_Event evt)
{
	bool success = true;

	if (evt.GetEventByteCount() != dmaSize)
	{
		TLOG(TLVL_ERROR) << "Event Header byte count does not match DMA byte count!";
		return false;
	}

	if (evt.GetHeader()->num_dtcs != evt.GetSubEventCount())
	{
		TLOG(TLVL_WARNING) << "Event Header num_dtcs field disagrees with number of SubEvents! (" << evt.GetHeader()->num_dtcs << " != " << evt.GetSubEventCount() << ")";
	}

	TLOG(TLVL_DEBUG + 3) << evt.GetHeader()->toJson();
	auto eventTag = evt.GetEventWindowTag();

	current_buffer_pos += sizeof(DTCLib::DTC_EventHeader);

	for (auto& subevt : evt.GetSubEvents())
	{
		auto subevtSuccess = VerifySubEvent(subevt, eventTag);
		success &= subevtSuccess;
		if (!continueFile) return false;
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
	total_size_read = 0;

	mu2e_databuff_t buf;

	bool success = true;
	bool continueFile = true;
	while (is && continueFile)
	{
		uint64_t dmaWriteSize;  // DMA Write buffer size
		is.read((char*)&dmaWriteSize, sizeof(dmaWriteSize));
		total_size_read += sizeof(dmaWriteSize);

		is.read((char*)&dmaSize, sizeof(dmaSize));
		total_size_read += sizeof(dmaSize);

		// Check that DMA Write Buffer Size = DMA Buffer Size + 16
		if (dmaSize + 16 != dmaWriteSize)
		{
			TLOG(TLVL_ERROR) << "Buffer error detected: DMA Size mismatch at 0x" << std::hex << total_size_read << ". Write size: " << dmaWriteSize << ", DMA Size: " << dmaSize;
			success = false;
			break;
		}

		// Check that size of all DataBlocks = DMA Buffer Size
		is.read((char*)buf, dmaSize);
		total_size_read += dmaSize;

		current_buffer_pos = 0;

		TLOG(TLVL_DEBUG + 1) << "Verifying event at offset 0x" << std::hex << (total_size_read - dmaSize);
		DTCLib::DTC_Event thisEvent(buf);
		success = VerifyEvent(thisEvent);
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
		VerifyFile(file);
	}
}
