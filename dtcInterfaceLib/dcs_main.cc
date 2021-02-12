// This file (util_main.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 13, 2015. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <unistd.h>  // usleep
#include <chrono>
#include <cmath>
#include <cstdio>   // printf
#include <cstdlib>  // strtoul
#include <iomanip>
#include <iostream>
#include <string>

#include "TRACE/tracemf.h"
#define TRACE_NAME "rocUtil"

#include "DTC.h"

#define DCS_TLVL(b) b ? TLVL_DEBUG + 6 : TLVL_INFO

using namespace DTCLib;

unsigned getOptionValue(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0')
	{
		(*index)++;
		return strtoul((*argv)[*index], nullptr, 0);
	}
	auto offset = 2;
	if (arg[2] == '=')
	{
		offset = 3;
	}

	return strtoul(&arg[offset], nullptr, 0);
}

std::string getOptionString(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0')
	{
		(*index)++;
		return std::string((*argv)[*index]);
	}
	auto offset = 2;
	if (arg[2] == '=')
	{
		offset = 3;
	}

	return std::string(&arg[offset]);
}

void printHelpMsg()
{
	std::cout << "Usage: rocUtil [options] "
				 "[read_register,simple_read,reset_roc,write_register,read_extregister,write_extregister,test_read,read_release,"
				 "toggle_serdes,block_read,block_write,raw_block_read]"
			  << std::endl;
	std::cout << "Options are:" << std::endl
			  << "    -h: This message." << std::endl
			  << "    -l: Link to send requests on (Default: 0)" << std::endl
			  << "    -n: Number of times to repeat test. (Default: 1)" << std::endl
			  << "    -d: Delay between tests, in us (Default: 0)." << std::endl
			  << "    -w: Data to write to address" << std::endl
			  << "    -a: Address to write" << std::endl
			  << "    -b: Block address (for write_rocext)" << std::endl
			  << "    -q: Quiet mode (Don't print requests)" << std::endl
			  << "    -Q: Really Quiet mode (Try not to print anything)" << std::endl
			  << "    -v: Expected DTC Design version string (Default: \"\")" << std::endl
			  << "    -c: Word count for Block Reads (Default: 0)" << std::endl
			  << "    -i: Do not set the incrementAddress bit for block operations" << std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	auto quiet = false;
	auto reallyQuiet = false;
	unsigned link = 0;
	unsigned delay = 0;
	unsigned number = 1;
	unsigned address = 0;
	unsigned data = 0;
	unsigned block = 0;
	size_t count = 0;
	bool incrementAddress = true;
	std::string op = "";

	for (auto optind = 1; optind < argc; ++optind)
	{
		if (argv[optind][0] == '-')
		{
			switch (argv[optind][1])
			{
				case 'l':
					link = getOptionValue(&optind, &argv);
					break;
				case 'd':
					delay = getOptionValue(&optind, &argv);
					break;
				case 'n':
					number = getOptionValue(&optind, &argv);
					break;
				case 'w':
					data = getOptionValue(&optind, &argv);
					break;
				case 'a':
					address = getOptionValue(&optind, &argv);
					break;
				case 'b':
					block = getOptionValue(&optind, &argv);
					break;
				case 'c':
					count = getOptionValue(&optind, &argv);
					break;
				case 'i':
					incrementAddress = !incrementAddress;
					break;
				case 'q':
					quiet = true;
					break;
				case 'Q':
					quiet = true;
					reallyQuiet = true;
					break;
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
			op = std::string(argv[optind]);
		}
	}

	if (op == "simple_read")
	{
		TRACE_CNTL("modeS", 0);
	}

	TLOG(TLVL_DEBUG) << "Options are: " << std::boolalpha
					 << "Operation: " << std::string(op) << ", Num: " << number << ", Link: " << link << ", Delay: " << delay
					 << ", Address: " << address << ", Data: " << data << ", Block: " << block << ", Quiet Mode: " << quiet
					 << ", Really Quiet Mode: " << reallyQuiet << std::endl;

	auto dtc_link = static_cast<DTC_Link_ID>(link);
	auto thisDTC = new DTC(DTC_SimMode_NoCFO, -1, (0x1 << (link * 4)));  // rocMask is in hex, not binary
	auto device = thisDTC->GetDevice();

	if (op == "read_register")
	{
		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG(TLVL_DEBUG) << "Operation \"read_register\" " << ii << std::endl;
			try
			{
				auto rocdata = thisDTC->ReadROCRegister(dtc_link, address);
				TLOG(DCS_TLVL(reallyQuiet)) << "ROC " << dtc_link << " returned " << rocdata << " for address " << address;
			}
			catch (std::runtime_error& err)
			{
				TLOG(TLVL_ERROR) << "Error reading from ROC: " << err.what();
			}
		}
	}
	else if (op == "simple_read")
	{
		TLOG(TLVL_DEBUG) << "Operation \"simple_read\"" << std::endl;
		try
		{
			auto rocdata = thisDTC->ReadROCRegister(dtc_link, address);
			std::cout << std::hex << std::showbase << rocdata;
			TLOG(DCS_TLVL(reallyQuiet)) << "ROC " << dtc_link << " returned " << rocdata << " for address " << address;
		}
		catch (std::runtime_error& err)
		{
			TLOG(TLVL_ERROR) << "Error reading from ROC: " << err.what();
		}
	}
	else if (op == "reset_roc")
	{
		TLOG(TLVL_DEBUG) << "Operation \"reset_roc\"" << std::endl;
		thisDTC->WriteExtROCRegister(dtc_link, 12, 1, 0x11, false);
		thisDTC->WriteExtROCRegister(dtc_link, 11, 1, 0x11, false);
		thisDTC->WriteExtROCRegister(dtc_link, 10, 1, 0x11, false);
		thisDTC->WriteExtROCRegister(dtc_link, 9, 1, 0x11, false);
		thisDTC->WriteExtROCRegister(dtc_link, 8, 1, 0x11, false);
	}
	else if (op == "write_register")
	{
		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG(TLVL_DEBUG) << "Operation \"write_register\" " << ii << std::endl;
			thisDTC->WriteROCRegister(dtc_link, address, data, false);
		}
	}
	else if (op == "read_extregister")
	{
		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG(TLVL_DEBUG) << "Operation \"read_register\" " << ii << std::endl;
			auto rocdata = thisDTC->ReadExtROCRegister(dtc_link, block, address);
			TLOG(DCS_TLVL(reallyQuiet)) << "ROC " << dtc_link << " returned " << rocdata << " for address " << address << ", block " << block;
		}
	}
	else if (op == "write_extregister")
	{
		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG(TLVL_DEBUG) << "Operation \"write_extregister\" " << ii << std::endl;
			thisDTC->WriteExtROCRegister(dtc_link, block, address, data, false);
		}
	}
	else if (op == "test_read")
	{
		TLOG(TLVL_DEBUG) << "Operation \"test_read\"" << std::endl;

		// thisDTC->SendDCSRequestPacket(dtc_link,  DTC_DCSOperationType_Read, address, quiet);

		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG(DCS_TLVL(reallyQuiet)) << "Buffer Read " << ii << std::endl;
			mu2e_databuff_t* buffer;
			auto tmo_ms = 1500;
			auto sts = device->read_data(DTC_DMA_Engine_DCS, reinterpret_cast<void**>(&buffer), tmo_ms);

			TLOG(TLVL_DEBUG) << "util - read for DCS - ii=" << ii << ", sts=" << sts << ", buffer=" << (void*)buffer;
			if (sts > 0)
			{
				auto bufSize = *reinterpret_cast<uint64_t*>(&buffer[0]);
				TLOG(TLVL_DEBUG) << "util - bufSize is " << bufSize;

				if (!reallyQuiet)
				{
					for (unsigned line = 0; line < static_cast<unsigned>(ceil((bufSize - 8) / 16)); ++line)
					{
						TLOG(TLVL_INFO) << "0x" << std::hex << std::setw(5) << std::setfill('0') << line << "0: ";
						// for (unsigned byte = 0; byte < 16; ++byte)
						for (unsigned byte = 0; byte < 8; ++byte)
						{
							if (line * 16 + 2 * byte < bufSize - 8u)
							{
								auto thisWord = reinterpret_cast<uint16_t*>(buffer)[4 + line * 8 + byte];
								// uint8_t thisWord = (((uint8_t*)buffer)[8 + (line * 16) + byte]);
								TLOG(TLVL_INFO) << std::setw(4) << static_cast<int>(thisWord) << " ";
							}
						}
						TLOG(TLVL_INFO) << std::endl;
					}
				}
			}
			device->read_release(DTC_DMA_Engine_DCS, 1);
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "toggle_serdes")
	{
		if (!thisDTC->ReadSERDESOscillatorClock())
		{
			TLOG(TLVL_INFO) << "Setting SERDES Oscillator Clock to 2.5 Gbps" << std::endl;
			thisDTC->SetSERDESOscillatorClock(DTC_SerdesClockSpeed_25Gbps);
		}
		else
		{
			TLOG(TLVL_INFO) << "Setting SERDES Oscillator Clock to 3.125 Gbps" << std::endl;
			thisDTC->SetSERDESOscillatorClock(DTC_SerdesClockSpeed_3125Gbps);
		}
	}
	else if (op == "read_release")
	{
		TLOG(TLVL_DEBUG) << "Operation \"read_release\"";
		for (unsigned ii = 0; ii < number; ++ii)
		{
			void* buffer;
			auto tmo_ms = 0;
			auto stsRD = device->read_data(DTC_DMA_Engine_DCS, &buffer, tmo_ms);
			auto stsRL = device->read_release(DTC_DMA_Engine_DCS, 1);
			TLOG(TLVL_DEBUG + 7) << "dcs - release/read for DCS ii=" << ii << ", stsRD=" << stsRD << ", stsRL=" << stsRL << ", buffer=" << buffer;
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "block_read")
	{
		std::vector<uint16_t> data(count);
		thisDTC->ReadROCBlock(data, dtc_link, address, count, incrementAddress);
		DTCLib::Utilities::PrintBuffer(&data[0], data.size() * sizeof(uint16_t));
	}
	else if (op == "block_write")
	{
		std::vector<uint16_t> blockData;
		for (size_t ii = 0; ii < count; ++ii)
		{
			blockData.push_back(static_cast<uint16_t>(ii));
		}
		thisDTC->WriteROCBlock(dtc_link, address, blockData, false, incrementAddress);
	}
	else if (op == "raw_block_read")
	{
		DTC_DCSRequestPacket req(dtc_link, DTC_DCSOperationType_BlockRead, false, incrementAddress, address, count);

		TLOG(TLVL_TRACE) << "rocUtil raw_block_read: before WriteDMADCSPacket - DTC_DCSRequestPacket";

		thisDTC->ReleaseAllBuffers(DTC_DMA_Engine_DCS);

		if (!thisDTC->ReadDCSReception()) thisDTC->EnableDCSReception();

		thisDTC->WriteDMAPacket(req);
		TLOG(TLVL_TRACE) << "rocUtil raw_block_read: after  WriteDMADCSPacket - DTC_DCSRequestPacket";

		usleep(2500);

		mu2e_databuff_t* buffer;
		auto tmo_ms = 1500;
		TLOG(TLVL_TRACE) << "rocUtil - before read for DCS";
		auto sts = device->read_data(DTC_DMA_Engine_DCS, reinterpret_cast<void**>(&buffer), tmo_ms);
		TLOG(TLVL_TRACE) << "rocUtil - after read for DCS - "
						 << " sts=" << sts << ", buffer=" << (void*)buffer;

		if (sts > 0)
		{
			void* readPtr = &buffer[0];
			auto bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
			readPtr = static_cast<uint8_t*>(readPtr) + 8;
			TLOG((reallyQuiet ? TLVL_DEBUG + 5 : TLVL_INFO)) << "Buffer reports DMA size of " << std::dec << bufSize << " bytes. Device driver reports read of "
															 << sts << " bytes," << std::endl;

			TLOG(TLVL_TRACE) << "util - bufSize is " << bufSize;

			if (!reallyQuiet)
			{
				DTCLib::Utilities::PrintBuffer(buffer, sts >= bufSize ? sts : bufSize);
			}
		}
		device->read_release(DTC_DMA_Engine_DCS, 1);
		if (delay > 0) usleep(delay);
	}
	else
	{
		TLOG(TLVL_ERROR) << "Unrecognized operation: " << op << std::endl;
		printHelpMsg();
	}

	delete thisDTC;

	if (op == "simple_read")
	{
		TRACE_CNTL("modeS", 1);
	}
	return 0;
}  // main
