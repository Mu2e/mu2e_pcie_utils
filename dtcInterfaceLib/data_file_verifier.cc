// This file (util_main.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 13, 2015. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <vector>
#include <fstream>

#include "mu2e_driver/mu2e_mmap_ioctl.h"

#include "TRACE/tracemf.h"
#define TRACE_NAME "data_file_verifier"

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

	for (auto& file : binaryFiles) {
		std::ifstream is(file);
		if (is.bad() || !is) {
			TLOG(TLVL_ERROR) << "Cannot read file " << file;
			continue;
		}
		TLOG(TLVL_INFO) << "Reading binary file " << file;
		uint64_t total_size_read = 0;

		mu2e_databuff_t buf;

		bool continueFile = true;
		while (is && continueFile) {

			uint64_t dmaWriteSize; // DMA Write buffer size
			is.read((char*)&dmaWriteSize, sizeof(dmaWriteSize));
			total_size_read += sizeof(dmaWriteSize);

			uint64_t dmaSize; // DMA buffer size
			is.read((char*)&dmaSize, sizeof(dmaSize));
			total_size_read += sizeof(dmaSize);


			// Check that DMA Write Buffer Size = DMA Buffer Size + 16
			if (dmaSize + 16 != dmaWriteSize) {
				TLOG(TLVL_ERROR) << "Buffer error detected: DMA Size mismatch at 0x" << std::hex << total_size_read << ". Write size: " << dmaWriteSize << ", DMA Size: " << dmaSize;
				break;
			}

			// Check that size of all DataBlocks = DMA Buffer Size
			is.read((char*)buf, dmaSize);
			total_size_read += dmaSize;

			size_t offset = 0;
			while (offset < dmaSize) {
				auto blockByteSize = *reinterpret_cast<uint16_t*>(buf + offset);

				// Check that this is indeed a DataHeader packet
				auto dataHeaderMask = 0x80F0;
				auto dataHeaderTest = *(reinterpret_cast<uint16_t*>(buf + offset) + 1);
				TLOG(TLVL_TRACE) << "Block size: 0x" <<std::hex << blockByteSize << ", Test word: " << std::hex << dataHeaderTest << ", masked: " << (dataHeaderTest & dataHeaderMask) << " =?= 0x8050";
				if ((dataHeaderTest & dataHeaderMask) != 0x8050) {
					TLOG(TLVL_ERROR) << "Encountered bad data at 0x" << std::hex << (total_size_read - dmaSize + offset) << ": expected DataHeader, got 0x" << std::hex << *reinterpret_cast<uint64_t*>(buf + offset);
					TLOG(TLVL_ERROR) << "This most likely means that the declared DMA size is incorrect, it was declared as 0x" << std::hex << dmaSize << ", but we ran out of DataHeaders at 0x" << std::hex << offset;
					// go to next file
					continueFile = false;
					break;
				}
				if (offset + blockByteSize > dmaSize) {
					TLOG(TLVL_ERROR) << "Block goes past end of DMA! Blocks should always end at DMA boundary! Error at 0x" << std::hex << (total_size_read - dmaSize + offset);
					// go to next file
					continueFile = false;
					break;
				}
				offset += blockByteSize;
			}
		}
	}
}