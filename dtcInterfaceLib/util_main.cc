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
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>

#include "TRACE/tracemf.h"
#define TRACE_NAME "mu2eUtil"

#include "DTC.h"
#include "DTCSoftwareCFO.h"

using namespace DTCLib;

bool incrementTimestamp = true;
bool syncRequests = false;
bool checkSERDES = false;
bool quiet = false;
unsigned quietCount = 0;
bool reallyQuiet = false;
bool rawOutput = false;
bool skipVerify = false;
bool writeDMAHeadersToOutput = false;
bool useCFOEmulator = true;
bool forceNoDebug = false;
unsigned genDMABlocks = 0;
std::string rawOutputFile = "/tmp/mu2eUtil.raw";
std::string expectedDesignVersion = "";
std::string simFile = "";
std::string timestampFile = "";
bool useSimFile = false;
unsigned delay = 0;
unsigned cfodelay = 1000;
unsigned number = 1;
unsigned long timestampOffset = 1;
unsigned eventCount = 1;
unsigned blockCount = 1;
unsigned packetCount = 0;
int requestsAhead = 0;
std::string op = "";
DTC_DebugType debugType = DTC_DebugType_SpecialSequence;
bool stickyDebugType = true;
int val = 0;
bool readGenerated = false;
std::ofstream outputStream;
unsigned rocMask = 0x1;
unsigned targetFrequency = 166666667;
int clockToProgram = 0;

#define __SHORTFILE__ \
	(strstr(&__FILE__[0], "/srcs/") ? strstr(&__FILE__[0], "/srcs/") + 6 : __FILE__)
#define __COUT__ std::cout << __SHORTFILE__ << " [" << std::dec << __LINE__ << "]\t"
#define __E__ std::endl
#define Q(X) #X
#define QUOTE(X) Q(X)
#define __COUTV__(X) __COUT__ << QUOTE(X) << " = " << X << __E__

int dtc = -1;

unsigned getOptionValue(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0')
	{
		(*index)++;
		unsigned ret = strtoul((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}
	auto offset = 2;
	if (arg[2] == '=')
	{
		offset = 3;
	}

	return strtoul(&arg[offset], nullptr, 0);
}
unsigned long long getOptionValueLong(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0')
	{
		(*index)++;
		unsigned long long ret = strtoull((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}
	auto offset = 2;
	if (arg[2] == '=')
	{
		offset = 3;
	}

	return strtoull(&arg[offset], nullptr, 0);
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

unsigned getLongOptionValue(int* index, char** argv[])
{
	auto arg = std::string((*argv)[*index]);
	auto pos = arg.find('=');

	if (pos == std::string::npos)
	{
		(*index)++;
		unsigned ret = strtoul((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}

	return strtoul(&arg[++pos], nullptr, 0);
}
unsigned long long getLongOptionValueLong(int* index, char** argv[])
{
	auto arg = std::string((*argv)[*index]);
	auto pos = arg.find('=');

	if (pos == std::string::npos)
	{
		(*index)++;
		unsigned long long ret = strtoull((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}

	return strtoull(&arg[++pos], nullptr, 0);
}

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

std::string getLongOptionString(int* index, char** argv[])
{
	auto arg = std::string((*argv)[*index]);

	if (arg.find('=') == std::string::npos)
	{
		return std::string((*argv)[++(*index)]);
	}
	else
	{
		return arg.substr(arg.find('='));
	}
}

void WriteGeneratedData(DTC* thisDTC)
{
	TLOG(TLVL_INFO) << "Sending data to DTC" << std::endl;
	thisDTC->DisableDetectorEmulator();
	thisDTC->DisableDetectorEmulatorMode();
	thisDTC->SetDDRDataLocalStartAddress(0);
	thisDTC->ResetDDRReadAddress();
	thisDTC->ResetDDRWriteAddress();
	thisDTC->EnableDetectorEmulatorMode();
	thisDTC->SetDDRDataLocalEndAddress(0xFFFFFFFF);
	uint64_t total_size_written = 0;
	uint32_t end_address = 0;
	unsigned ii = 0;
	uint32_t packetCounter = 0;
	uint64_t ts = timestampOffset;
	for (; ii < genDMABlocks; ++ii)
	{
		auto blockByteCount = static_cast<uint16_t>((1 + packetCount) * 16 * sizeof(uint8_t));
		auto eventByteCount = static_cast<uint64_t>(blockCount * blockByteCount);             // Exclusive byte count
		auto eventWriteByteCount = static_cast<uint64_t>(eventByteCount + sizeof(uint64_t));  // Inclusive byte count
		auto dmaByteCount = static_cast<uint64_t>(eventWriteByteCount * eventCount);          // Exclusive byte count
		auto dmaWriteByteCount = dmaByteCount + sizeof(uint64_t);                             // Inclusive byte count

		if (dmaWriteByteCount > 0x7FFF)
		{
			TLOG(TLVL_ERROR) << "Requested DMA write is larger than the allowed size! Reduce event/block/packet counts!"
							 << std::endl;
			TLOG(TLVL_ERROR) << "Block Byte Count: " << std::hex << std::showbase << blockByteCount
							 << ", Event Byte Count: " << eventByteCount << ", DMA Write Count: " << dmaWriteByteCount
							 << ", MAX: 0x7FFF" << std::endl;
			exit(1);
		}

		auto buf = reinterpret_cast<mu2e_databuff_t*>(new char[0x10000]);
		memcpy(buf, &dmaWriteByteCount, sizeof(uint64_t));
		if (rawOutput && writeDMAHeadersToOutput)
			outputStream.write(reinterpret_cast<char*>(&dmaWriteByteCount), sizeof(uint64_t));
		auto currentOffset = sizeof(uint64_t);

		for (unsigned ll = 0; ll < eventCount; ++ll)
		{
			memcpy(reinterpret_cast<uint8_t*>(buf) + currentOffset, &eventByteCount, sizeof(uint64_t));
			if (rawOutput && writeDMAHeadersToOutput)
				outputStream.write(reinterpret_cast<char*>(&eventByteCount), sizeof(uint64_t));
			currentOffset += sizeof(uint64_t);

			if (incrementTimestamp) ++ts;

			for (unsigned kk = 0; kk < blockCount; ++kk)
			{
				auto index = kk % DTC_Links.size();
				DTC_DataHeaderPacket header(DTC_Links[index], static_cast<uint16_t>(packetCount), DTC_DataStatus_Valid,
											static_cast<uint8_t>(kk / DTC_Links.size()), DTC_Subsystem_Other, 0, DTC_Timestamp(ts));
				auto packet = header.ConvertToDataPacket();
				memcpy(reinterpret_cast<uint8_t*>(buf) + currentOffset, packet.GetData(), sizeof(uint8_t) * 16);
				if (rawOutput) outputStream << packet;
				currentOffset += 16;

				uint16_t dataPacket[8];
				for (unsigned jj = 0; jj < packetCount; ++jj)
				{
					if (currentOffset + 16 > sizeof(mu2e_databuff_t))
					{
						break;
					}

					dataPacket[0] = 0x4144;
					dataPacket[1] = 0x4154;
					dataPacket[2] = 0x4144;
					dataPacket[3] = 0x4154;
					packetCounter += 1;
					memcpy(&dataPacket[4], &packetCounter, sizeof(uint32_t));
					uint32_t tmp = jj + 1;
					memcpy(&dataPacket[6], &tmp, sizeof(uint32_t));

					memcpy(reinterpret_cast<uint8_t*>(buf) + currentOffset, &dataPacket[0], sizeof(uint8_t) * 16);
					if (rawOutput) outputStream.write(reinterpret_cast<char*>(&dataPacket[0]), 16);
					currentOffset += 16;
				}
			}
		}

		total_size_written += dmaWriteByteCount;
		end_address += static_cast<uint32_t>(dmaByteCount);

		if (!reallyQuiet)
		{
			TLOG(TLVL_INFO) << "Buffer " << ii << ":" << std::endl;
			DTCLib::Utilities::PrintBuffer(buf, dmaWriteByteCount, quietCount);
		}

		thisDTC->GetDevice()->write_data(DTC_DMA_Engine_DAQ, buf, static_cast<size_t>(dmaWriteByteCount));
		delete[] buf;
	}

	TLOG(TLVL_INFO) << "Total bytes written: " << std::dec << total_size_written << std::hex << "( 0x" << total_size_written
					<< " )" << std::endl;
	thisDTC->SetDDRDataLocalEndAddress(end_address - 1);
	if (readGenerated)
	{
		if (rawOutput) outputStream.close();
		exit(0);
	}
	thisDTC->SetDetectorEmulationDMACount(number);
	thisDTC->EnableDetectorEmulator();
}

void printHelpMsg()
{
	std::cout << "Usage: mu2eUtil [options] "
				 "[read,read_data,reset_ddrread,reset_detemu,toggle_serdes,loopback,buffer_test,read_release,DTC,program_"
				 "clock,verify_simfile,dma_info]"
			  << std::endl;
	std::cout
		<< "Options are:" << std::endl
		<< "    -h, --help: This message." << std::endl
		<< "    -n: Number of times to repeat test. (Default: 1)" << std::endl
		<< "    -o: Starting Timestamp offest. (Default: 1)." << std::endl
		<< "    -i: Do not increment Timestamps." << std::endl
		<< "    -S: Synchronous Timestamp mode (1 RR & DR per Read operation)" << std::endl
		<< "    -d: Delay between tests, in us (Default: 0)." << std::endl
		<< "    -D: CFO Request delay interval (Default: 1000 (minimum)." << std::endl
		<< "    -c: Number of Debug Packets to request (Default: 0)." << std::endl
		<< "    -N: Do NOT set the Debug flag in generated Data Request packets" << std::endl
		<< "    -b: Number of Data Blocks to generate per Event (Default: 1)." << std::endl
		<< "    -E: Number of Events to generate per DMA block (Default: 1)." << std::endl
		<< "    -a: Number of Readout Request/Data Requests to send before starting to read data (Default: 0)."
		<< std::endl
		<< "    -q: Quiet mode (Don't print requests) Additionally, for buffer_test mode, limits to N (Default 1) "
		   "packets at the beginning and end of the buffer."
		<< std::endl
		<< "    -Q: Really Quiet mode (Try not to print anything)" << std::endl
		<< "    -s: Stop on SERDES Error." << std::endl
		<< "    -e: Use DTCLib's SoftwareCFO instead of the DTC CFO Emulator" << std::endl
		<< "    -t: Use DebugType flag (1st request gets ExternalDataWithFIFOReset, the rest get ExternalData)"
		<< std::endl
		<< "    -T: Set DebugType flag for ALL requests (0, 1, or 2)" << std::endl
		<< "    -f: RAW Output file path" << std::endl
		<< "    -H: Write DMA headers to raw output file (when -f is used with -g)" << std::endl
		<< "    -p: Send DTCLIB_SIM_FILE to DTC and enable Detector Emulator mode" << std::endl
		<< "    -P: Send <file> to DTC and enable Detector Emulator mode (Default: \"\")" << std::endl
		<< "    -g: Generate (and send) N DMA blocks for testing the Detector Emulator (Default: 0)" << std::endl
		<< "    -G: Read out generated data, but don't write new. With -g, will exit after writing data" << std::endl
		<< "    -r: # of rocs to enable. Hexadecimal, each digit corresponds to a link. ROC_0: 1, ROC_1: 3, ROC_2: 5, "
		   "ROC_3: 7, ROC_4: 9, ROC_5: B (Default 0x1, All possible: 0xBBBBBB)"
		<< std::endl
		<< "    -F: Frequency to program (in Hz, sorry...Default 166666667 Hz)" << std::endl
		<< "    -C: Clock to program (0: SERDES, 1: DDR, 2: Timing, Default 0)" << std::endl
		<< "    -v: Expected DTC Design version string (Default: \"\")" << std::endl
		<< "    -V: Do NOT attempt to verify that the sim file landed in DTC memory correctly" << std::endl
		<< "    --timestamp-list: Read <file> for timestamps to request (CFO will generate heartbeats for all timestamps "
		   "in range spanned by file)"
		<< std::endl
		<< "    --dtc: Use dtc <num> (Defaults to DTCLIB_DTC if set, 0 otherwise, see ls /dev/mu2e* for available DTCs)"
		<< std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	for (auto optind = 1; optind < argc; ++optind)
	{
		if (argv[optind][0] == '-')
		{
			switch (argv[optind][1])
			{
				case 'i':
					incrementTimestamp = false;
					break;
				case 'd':
					delay = getOptionValue(&optind, &argv);
					break;
				case 'D':
					cfodelay = getOptionValue(&optind, &argv);
					break;
				case 'S':
					syncRequests = true;
					break;
				case 'n':
					number = getOptionValue(&optind, &argv);
					break;
				case 'o':
					timestampOffset = getOptionValueLong(&optind, &argv) & 0x0000FFFFFFFFFFFF;  // Timestamps are 48 bits
					break;
				case 'c':
					packetCount = getOptionValue(&optind, &argv);
					break;
				case 'N':
					forceNoDebug = true;
					break;
				case 'b':
					blockCount = getOptionValue(&optind, &argv);
					break;
				case 'E':
					eventCount = getOptionValue(&optind, &argv);
					break;
				case 'a':
					requestsAhead = getOptionValue(&optind, &argv);
					break;
				case 'q':
					quiet = true;
					quietCount = getOptionValue(&optind, &argv);
					break;
				case 'p':
					useSimFile = true;
					break;
				case 'P':
					useSimFile = true;
					simFile = getOptionValue(&optind, &argv);
					break;
				case 'g':
					genDMABlocks = getOptionValue(&optind, &argv);
					break;
				case 'G':
					readGenerated = true;
					break;
				case 'Q':
					quiet = true;
					reallyQuiet = true;
					break;
				case 's':
					checkSERDES = true;
					break;
				case 'e':
					useCFOEmulator = false;
					break;
				case 'f':
					rawOutput = true;
					rawOutputFile = getOptionString(&optind, &argv);
					break;
				case 'H':
					writeDMAHeadersToOutput = true;
					break;
				case 't':
					debugType = DTC_DebugType_ExternalSerialWithReset;
					stickyDebugType = false;
					break;
				case 'T':
					val = getOptionValue(&optind, &argv);
					if (val < static_cast<int>(DTC_DebugType_Invalid))
					{
						stickyDebugType = true;
						debugType = static_cast<DTC_DebugType>(val);
						break;
					}
					TLOG(TLVL_ERROR) << "Invalid Debug Type passed to -T!" << std::endl;
					printHelpMsg();
					break;
				case 'r':
					rocMask = getOptionValue(&optind, &argv);
					break;
				case 'C':
					clockToProgram = getOptionValue(&optind, &argv) % 3;
					break;
				case 'F':
					targetFrequency = getOptionValue(&optind, &argv);
					break;
				case 'v':
					expectedDesignVersion = getOptionString(&optind, &argv);
					break;
				case 'V':
					skipVerify = true;
					break;
				case '-':  // Long option
				{
					auto option = getLongOptionOption(&optind, &argv);
					if (option == "--timestamp-list")
					{
						timestampFile = getLongOptionString(&optind, &argv);
					}
					else if (option == "--dtc")
					{
						dtc = getLongOptionValue(&optind, &argv);
					}
					else if (option == "--help")
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
			op = std::string(argv[optind]);
		}
	}

	TLOG(TLVL_DEBUG) << "Options are: " << std::boolalpha
					 << "Operation: " << std::string(op) << ", DTC: " << dtc << ", Num: " << number << ", Delay: " << delay
					 << ", CFO Delay: " << cfodelay << ", TS Offset: " << timestampOffset << ", PacketCount: " << packetCount
					 << ", Force NO Debug Flag: " << forceNoDebug << ", DataBlock Count: " << blockCount;
	TLOG(TLVL_DEBUG) << std::boolalpha << ", Event Count: " << eventCount << ", Requests Ahead of Reads: " << requestsAhead
					 << ", Synchronous Request Mode: " << syncRequests << ", Use DTC CFO Emulator: " << useCFOEmulator
					 << ", Increment TS: " << incrementTimestamp << ", Quiet Mode: " << quiet << " (" << quietCount << ")"
					 << ", Really Quiet Mode: " << reallyQuiet << ", Check SERDES Error Status: " << checkSERDES;
	TLOG(TLVL_DEBUG) << std::boolalpha << ", Generate DMA Blocks: " << genDMABlocks << ", Read Data from DDR: " << readGenerated
					 << ", Use Sim File: " << useSimFile << ", Skip Verify: " << skipVerify << ", ROC Mask: " << std::hex
					 << rocMask << ", Debug Type: " << DTC_DebugTypeConverter(debugType).toString()
					 << ", Target Frequency: " << std::dec << targetFrequency;
	TLOG(TLVL_DEBUG) << std::boolalpha << ", Clock To Program: " << (clockToProgram == 0 ? "SERDES" : (clockToProgram == 1 ? "DDR" : "Timing"))
					 << ", Expected Design Version: " << expectedDesignVersion;
	if (rawOutput)
	{
		TLOG(TLVL_DEBUG) << ", Raw output file: " << rawOutputFile;
	}
	if (simFile.size() > 0)
	{
		TLOG(TLVL_DEBUG) << ", Sim file: " << simFile;
	}
	if (rawOutput) outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);

	if (op == "read")
	{
		TLOG(TLVL_DEBUG) << "Operation \"read\"" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		auto packet = thisDTC->ReadNextDAQPacket();
		if (packet)
		{
			TLOG((reallyQuiet ? 10 : TLVL_INFO)) << packet->toJSON() << '\n';
			if (rawOutput)
			{
				auto rawPacket = packet->ConvertToDataPacket();
				for (auto ii = 0; ii < 16; ++ii)
				{
					auto word = rawPacket.GetWord(ii);
					outputStream.write(reinterpret_cast<char*>(&word), sizeof(uint8_t));
				}
			}
		}
		delete thisDTC;
	}
	else if (op == "read_data")
	{
		TLOG(TLVL_DEBUG) << "Operation \"read_data\"" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);

		auto device = thisDTC->GetDevice();
		if (readGenerated)
		{
			thisDTC->EnableDetectorEmulatorMode();
			thisDTC->SetDetectorEmulationDMACount(number);
			thisDTC->EnableDetectorEmulator();
		}
		for (unsigned ii = 0; ii < number; ++ii)
		{
			TLOG((reallyQuiet ? 9 : TLVL_INFO)) << "Buffer Read " << ii << std::endl;
			mu2e_databuff_t* buffer;
			auto tmo_ms = 1500;
			auto sts = device->read_data(DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);

			TLOG(TLVL_TRACE) << "util - read for DAQ - ii=" << ii << ", sts=" << sts << ", buffer=" << (void*)buffer;
			if (sts > 0)
			{
				auto bufSize = static_cast<uint16_t>(*reinterpret_cast<uint64_t*>(&buffer[0]));
				TLOG(TLVL_TRACE) << "util - bufSize is " << bufSize;

				if (!reallyQuiet)
				{
					DTCLib::Utilities::PrintBuffer(buffer, bufSize, quietCount);
				}
			}
			device->read_release(DTC_DMA_Engine_DAQ, 1);
			if (delay > 0) usleep(delay);
		}
		delete thisDTC;
	}
	else if (op == "toggle_serdes")
	{
		TLOG(TLVL_DEBUG) << "Swapping SERDES Oscillator Clock" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		auto clock = thisDTC->ReadSERDESOscillatorClock();
		if (clock == DTC_SerdesClockSpeed_3125Gbps)
		{
			TLOG(TLVL_INFO) << "Setting SERDES Oscillator Clock to 2.5 Gbps" << std::endl;
			thisDTC->SetSERDESOscillatorClock(DTC_SerdesClockSpeed_25Gbps);
		}
		else if (clock == DTC_SerdesClockSpeed_25Gbps)
		{
			TLOG(TLVL_INFO) << "Setting SERDES Oscillator Clock to 3.125 Gbps" << std::endl;
			thisDTC->SetSERDESOscillatorClock(DTC_SerdesClockSpeed_3125Gbps);
		}
		else
		{
			TLOG(TLVL_ERROR) << "Error: SERDES clock not recognized value!";
		}
		delete thisDTC;
	}
	else if (op == "reset_ddrread")
	{
		TLOG(TLVL_DEBUG) << "Resetting DDR Read Address" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		thisDTC->ResetDDRReadAddress();
		delete thisDTC;
	}
	else if (op == "reset_detemu")
	{
		TLOG(TLVL_DEBUG) << "Resetting Detector Emulator" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		thisDTC->ClearDetectorEmulatorInUse();
		thisDTC->ResetDDR();
		thisDTC->ResetDTC();
		delete thisDTC;
	}
	else if (op == "verify_simfile")
	{
		TLOG(TLVL_DEBUG) << "Operation \"verify_simfile\"" << std::endl;
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		auto device = thisDTC->GetDevice();

		device->ResetDeviceTime();

		if (useSimFile)
		{
			thisDTC->DisableDetectorEmulator();
			thisDTC->EnableDetectorEmulatorMode();
			thisDTC->VerifySimFileInDTC(simFile, rawOutputFile);
		}
	}
	else if (op == "buffer_test")
	{
		TLOG(TLVL_DEBUG) << "Operation \"buffer_test\"" << std::endl;
		auto startTime = std::chrono::steady_clock::now();
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);
		auto device = thisDTC->GetDevice();

		auto initTime = device->GetDeviceTime();
		device->ResetDeviceTime();
		auto afterInit = std::chrono::steady_clock::now();

		DTCSoftwareCFO cfo(thisDTC, useCFOEmulator, packetCount, debugType, stickyDebugType, quiet, false, forceNoDebug);

		if (genDMABlocks > 0)
		{
			WriteGeneratedData(thisDTC);
		}
		else if (useSimFile)
		{
			auto overwrite = false;
			if (simFile.size() > 0) overwrite = true;
			thisDTC->WriteSimFileToDTC(simFile, false, overwrite, rawOutputFile, skipVerify);
			if (readGenerated)
			{
				exit(0);
			}
		}
		else if (readGenerated)
		{
			thisDTC->DisableDetectorEmulator();
			thisDTC->EnableDetectorEmulatorMode();
			thisDTC->SetDetectorEmulationDMACount(number);
			thisDTC->EnableDetectorEmulator();
		}

		if (thisDTC->ReadSimMode() != DTC_SimMode_Loopback && timestampFile != "")
		{
			syncRequests = false;
			std::set<DTC_Timestamp> timestamps;
			std::ifstream is(timestampFile);
			uint64_t a;
			while (is >> a)
			{
				timestamps.insert(DTC_Timestamp(a));
			}
			number = timestamps.size();
			cfo.SendRequestsForList(timestamps, cfodelay);
		}
		else if (thisDTC->ReadSimMode() != DTC_SimMode_Loopback && !syncRequests)
		{
			cfo.SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, cfodelay, requestsAhead);
		}
		else if (thisDTC->ReadSimMode() == DTC_SimMode_Loopback)
		{
			uint64_t ts = timestampOffset;
			DTC_DataHeaderPacket header(DTC_Link_0, static_cast<uint16_t>(0), DTC_DataStatus_Valid, 0, DTC_Subsystem_Other, 0, DTC_Timestamp(ts));
			TLOG(TLVL_INFO) << "Request: " << header.toJSON() << std::endl;
			thisDTC->WriteDMAPacket(header);
		}

		auto readoutRequestTime = device->GetDeviceTime();
		device->ResetDeviceTime();
		auto afterRequests = std::chrono::steady_clock::now();

		for (unsigned ii = 0; ii < number; ++ii)
		{
			if (syncRequests)
			{
				auto startRequest = std::chrono::steady_clock::now();
				cfo.SendRequestForTimestamp(DTC_Timestamp(timestampOffset + (incrementTimestamp ? ii : 0)));
				auto endRequest = std::chrono::steady_clock::now();
				readoutRequestTime +=
					std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(endRequest - startRequest).count();
			}
			TLOG((reallyQuiet ? 9 : TLVL_INFO)) << "Buffer Read " << std::dec << ii << std::endl;

			__COUT__ << "Buffer read " << ii << __E__;

			mu2e_databuff_t* buffer;
			auto tmo_ms = 1500;
			TLOG(TLVL_TRACE) << "util - before read for DAQ - ii=" << ii;
			auto sts = device->read_data(DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
			TLOG(TLVL_TRACE) << "util - after read for DAQ - ii=" << ii << ", sts=" << sts << ", buffer=" << (void*)buffer;

			if (sts > 0)
			{
				void* readPtr = &buffer[0];
				auto bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
				readPtr = static_cast<uint8_t*>(readPtr) + 8;
				TLOG((reallyQuiet ? 9 : TLVL_INFO)) << "Buffer reports DMA size of " << std::dec << bufSize << " bytes. Device driver reports read of "
													<< sts << " bytes," << std::endl;

				TLOG(TLVL_TRACE) << "util - bufSize is " << bufSize;
				if (rawOutput) outputStream.write(static_cast<char*>(readPtr), sts - 8);

				if (!reallyQuiet)
				{
					DTCLib::Utilities::PrintBuffer(buffer, sts, quietCount);
				}
			}
			else if (checkSERDES)
				break;
			device->read_release(DTC_DMA_Engine_DAQ, 1);
			if (delay > 0) usleep(delay);
		}

		auto readDevTime = device->GetDeviceTime();
		auto doneTime = std::chrono::steady_clock::now();
		auto totalBytesRead = device->GetReadSize();
		auto totalBytesWritten = device->GetWriteSize();
		auto totalTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(doneTime - startTime).count();
		auto totalInitTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(afterInit - startTime).count();
		auto totalRequestTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(afterRequests - afterInit).count();
		auto totalReadTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(doneTime - afterRequests).count();

		TLOG(TLVL_INFO) << "Total Elapsed Time: " << Utilities::FormatTimeString(totalTime) << "." << std::endl
						<< "Total Init Time: " << Utilities::FormatTimeString(totalInitTime) << "." << std::endl
						<< "Total Readout Request Time: " << Utilities::FormatTimeString(totalRequestTime) << "." << std::endl
						<< "Total Read Time: " << Utilities::FormatTimeString(totalReadTime) << "." << std::endl;
		TLOG(TLVL_INFO) << "Device Init Time: " << Utilities::FormatTimeString(initTime) << "." << std::endl
						<< "Device Request Time: " << Utilities::FormatTimeString(readoutRequestTime) << "." << std::endl
						<< "Device Read Time: " << Utilities::FormatTimeString(readDevTime) << "." << std::endl;
		TLOG(TLVL_INFO) << "Total Bytes Written: " << Utilities::FormatByteString(static_cast<double>(totalBytesWritten), "")
						<< "." << std::endl
						<< "Total Bytes Read: " << Utilities::FormatByteString(static_cast<double>(totalBytesRead), "") << "."
						<< std::endl;
		TLOG(TLVL_INFO) << "Total PCIe Rate: "
						<< Utilities::FormatByteString((totalBytesWritten + totalBytesRead) / totalTime, "/s") << std::endl
						<< "Read Rate: " << Utilities::FormatByteString(totalBytesRead / totalReadTime, "/s") << std::endl
						<< "Device Read Rate: " << Utilities::FormatByteString(totalBytesRead / readDevTime, "/s") << std::endl;

		delete thisDTC;
	}
	else if (op == "read_release")
	{
		TLOG(TLVL_DEBUG) << "Operation \"read_release\"";
		mu2edev device;
		device.init(DTCLib::DTC_SimMode_Disabled, dtc);
		for (unsigned ii = 0; ii < number; ++ii)
		{
			void* buffer;
			auto tmo_ms = 0;
			auto stsRD = device.read_data(DTC_DMA_Engine_DAQ, &buffer, tmo_ms);
			auto stsRL = device.read_release(DTC_DMA_Engine_DAQ, 1);
			TLOG(12) << "util - release/read for DAQ and DCS ii=" << ii << ", stsRD=" << stsRD << ", stsRL=" << stsRL << ", buffer=" << buffer;
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "DTC")
	{
		TLOG(TLVL_DEBUG) << "Operation \"DTC\"";
		auto startTime = std::chrono::steady_clock::now();
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion);

		auto initTime = thisDTC->GetDevice()->GetDeviceTime();
		thisDTC->GetDevice()->ResetDeviceTime();
		auto afterInit = std::chrono::steady_clock::now();

		DTCSoftwareCFO theCFO(thisDTC, useCFOEmulator, packetCount, debugType, stickyDebugType, quiet, forceNoDebug);

		if (genDMABlocks > 0)
		{
			WriteGeneratedData(thisDTC);
		}
		else if (useSimFile)
		{
			auto overwrite = false;
			if (simFile.size() > 0) overwrite = true;
			thisDTC->WriteSimFileToDTC(simFile, false, overwrite);
		}
		else if (readGenerated)
		{
			thisDTC->DisableDetectorEmulator();
			thisDTC->EnableDetectorEmulatorMode();
			thisDTC->SetDetectorEmulationDMACount(number);
			thisDTC->EnableDetectorEmulator();
		}

		if (!syncRequests)
		{
			theCFO.SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, cfodelay, requestsAhead);
		}

		uint64_t ii = 0;
		auto retries = 4;
		uint64_t expectedTS = timestampOffset;
		auto packetsProcessed = 0;

		auto afterRequests = std::chrono::steady_clock::now();
		auto readoutRequestTime = thisDTC->GetDevice()->GetDeviceTime();
		thisDTC->GetDevice()->ResetDeviceTime();

		for (; ii < number; ++ii)
		{
			TLOG((reallyQuiet ? 10 : TLVL_INFO)) << "util_main: DTC Read " << ii << ": ";
			if (syncRequests)
			{
				auto startRequest = std::chrono::steady_clock::now();
				auto ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
				theCFO.SendRequestForTimestamp(DTC_Timestamp(ts));
				auto endRequest = std::chrono::steady_clock::now();
				readoutRequestTime +=
					std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(endRequest - startRequest).count();
			}

			auto data = thisDTC->GetData();  // DTC_Timestamp(ts));

			if (data.size() > 0)
			{
				TLOG(19) << data.size() << " DataBlocks returned";
				if (!reallyQuiet) TLOG(TLVL_INFO) << data.size() << " DataBlocks returned\n";
				packetsProcessed += static_cast<int>(data.size());
				for (size_t i = 0; i < data.size(); ++i)
				{
					TLOG(19) << "util_main constructing DataPacket:";
					auto test = DTC_DataPacket(data[i].blockPointer);
					// TRACE(19, test.toJSON().c_str());
					// if (!reallyQuiet) cout << test.toJSON() << '\n'; // dumps whole databuff_t
					auto h2 = DTC_DataHeaderPacket(test);
					if (expectedTS != h2.GetTimestamp().GetTimestamp(true))
					{
						TLOG(TLVL_INFO) << std::dec << h2.GetTimestamp().GetTimestamp(true) << " does not match expected timestamp of "
										<< expectedTS << "!!!" << std::endl;
						if (incrementTimestamp && h2.GetTimestamp().GetTimestamp(true) <= timestampOffset + number)
						{
							auto diff = static_cast<int64_t>(h2.GetTimestamp().GetTimestamp(true)) - static_cast<int64_t>(expectedTS);
							ii += diff > 0 ? diff : 0;
						}
						expectedTS = h2.GetTimestamp().GetTimestamp(true);
					}
					else if (i == data.size() - 1)
					{
						expectedTS += incrementTimestamp ? 1 : 0;
					}
					TLOG((reallyQuiet ? 19 : TLVL_INFO)) << h2.toJSON();
					if (rawOutput)
					{
						auto rawPacket = h2.ConvertToDataPacket();
						outputStream << rawPacket;
					}

					for (auto jj = 0; jj < h2.GetPacketCount(); ++jj)
					{
						auto packet = DTC_DataPacket(reinterpret_cast<uint8_t*>(data[i].blockPointer) + (jj + 1) * 16);
						TLOG((quiet ? 8 : TLVL_INFO)) << "\t" << packet.toJSON() << std::endl;
						if (rawOutput)
						{
							outputStream << packet;
						}
					}
				}
			}
			else
			{
				// TRACE_CNTL("modeM", 0L);
				TLOG((reallyQuiet ? 9 : TLVL_WARNING)) << "no data returned\n";
				// return (0);
				// break;
				usleep(100000);
				ii--;
				retries--;
				if (retries <= 0) break;
				continue;
			}
			retries = 4;

			if (checkSERDES)
			{
				auto disparity = thisDTC->ReadSERDESRXDisparityError(DTC_Link_0);
				auto cnit = thisDTC->ReadSERDESRXCharacterNotInTableError(DTC_Link_0);
				if (cnit.GetData()[0] || cnit.GetData()[1])
				{
					// TRACE_CNTL("modeM", 0L);
					TLOG(TLVL_WARNING) << "Character Not In Table Error detected" << std::endl;
					// return 0;
					break;
				}
				if (disparity.GetData()[0] || disparity.GetData()[1])
				{
					// TRACE_CNTL("modeM", 0L);
					TLOG(TLVL_WARNING) << "Disparity Error Detected" << std::endl;
					// return 0;
					break;
				}
			}
			if (delay > 0) usleep(delay);
		}

		auto readDevTime = thisDTC->GetDevice()->GetDeviceTime();
		auto doneTime = std::chrono::steady_clock::now();
		auto totalBytesRead = thisDTC->GetDevice()->GetReadSize();
		auto totalBytesWritten = thisDTC->GetDevice()->GetWriteSize();
		auto totalTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(doneTime - startTime).count();
		auto totalInitTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(afterInit - startTime).count();
		auto totalRequestTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(afterRequests - afterInit).count();
		auto totalReadTime =
			std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(doneTime - afterRequests).count();

		TLOG(TLVL_INFO) << "Total Elapsed Time: " << totalTime << " s." << std::endl
						<< "Total Init Time: " << totalInitTime << " s." << std::endl
						<< "Total Readout Request Time: " << totalRequestTime << " s." << std::endl
						<< "Total Read Time: " << totalReadTime << " s." << std::endl;
		TLOG(TLVL_INFO) << "Device Init Time: " << initTime << " s." << std::endl
						<< "Device Request Time: " << readoutRequestTime << " s." << std::endl
						<< "Device Read Time: " << readDevTime << " s." << std::endl;
		TLOG(TLVL_INFO) << "Total Bytes Written: " << Utilities::FormatByteString(static_cast<double>(totalBytesWritten), "")
						<< "." << std::endl
						<< "Total Bytes Read: " << Utilities::FormatByteString(static_cast<double>(totalBytesRead), "") << "."
						<< std::endl;
		TLOG(TLVL_INFO) << "Total PCIe Rate: "
						<< Utilities::FormatByteString((totalBytesWritten + totalBytesRead) / totalTime, "/s") << "." << std::endl
						<< "Read Rate: " << Utilities::FormatByteString(totalBytesRead / totalReadTime, "/s") << "." << std::endl
						<< "Device Read Rate: " << Utilities::FormatByteString(totalBytesRead / readDevTime, "/s") << "."
						<< std::endl;
		delete thisDTC;
	}
	else if (op == "program_clock")
	{
		TLOG(TLVL_DEBUG) << "Operation \"program_clock\"";
		auto thisDTC = new DTC(DTC_SimMode_NoCFO, dtc, rocMask, expectedDesignVersion, true);
		auto oscillator = clockToProgram == 0 ? DTC_OscillatorType_SERDES
											  : (clockToProgram == 1 ? DTC_OscillatorType_DDR : DTC_OscillatorType_Timing);
		thisDTC->SetNewOscillatorFrequency(oscillator, targetFrequency);
		delete thisDTC;
	}
	else if (op == "dma_info")
	{
		TLOG(TLVL_DEBUG) << "Opearation \"dma_info\"";
		if (dtc == -1)
		{
			auto dtcE = getenv("DTCLIB_DTC");
			if (dtcE != nullptr)
			{
				dtc = atoi(dtcE);
			}
			else
				dtc = 0;
		}

		mu2edev device;
		device.init(DTCLib::DTC_SimMode_Disabled, dtc);
		device.meta_dump();
	}
	else
	{
		TLOG(TLVL_ERROR) << "Unrecognized operation: " << op << std::endl;
		printHelpMsg();
	}

	if (rawOutput) outputStream.close();
	return 0;
}  // main
