// This file (util_main.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 13, 2015. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <cstdio>		// printf
#include <cstdlib>		// strtoul
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <chrono>
#include <cmath>
#include "DTC.h"
#include "DTCSoftwareCFO.h"
#ifdef _WIN32
# include <thread>
# define usleep(x)  std::this_thread::sleep_for(std::chrono::microseconds(x));
# ifndef TRACE
#  include <stdio.h>
#  ifdef _DEBUG
#   define TRACE(lvl,...) printf(__VA_ARGS__); printf("\n")
#  else
#   define TRACE(...)
#  endif
# endif
# define TRACE_CNTL(...)
#else
# include "trace.h"
# include <unistd.h>		// usleep
#endif
#define TRACE_NAME "MU2EDEV"

using namespace DTCLib;
using namespace std;

unsigned getOptionValue(int *index, char **argv[])
{
	char* arg = (*argv)[*index];
	if (arg[2] == '\0') {
		(*index)++;
		return strtoul((*argv)[*index], NULL, 0);
	}
	else {
		int offset = 2;
		if (arg[2] == '=') {
			offset = 3;
		}

		return strtoul(&(arg[offset]), NULL, 0);
	}
}

std::string getOptionString(int *index, char **argv[])
{
	char* arg = (*argv)[*index];
	if (arg[2] == '\0') {
		(*index)++;
		return std::string((*argv)[*index]);
	}
	else {
		int offset = 2;
		if (arg[2] == '=') {
			offset = 3;
		}

		return std::string(&(arg[offset]));
	}
}

void printHelpMsg() {
	cout << "Usage: mu2eUtil [options] [read,read_data,toggle_serdes,buffer_test,read_release,HW,DTC]" << endl;
	cout << "Options are:" << endl
		<< "    -h: This message." << endl
		<< "    -n: Number of times to repeat test. (Default: 1)" << endl
		<< "    -o: Starting Timestamp offest. (Default: 1)." << endl
		<< "    -i: Do not increment Timestamps." << endl
		<< "    -d: Delay between tests, in us (Default: 0)." << endl
		<< "    -c: Number of Debug Packets to request (Default: 0)." << endl
		<< "    -a: Number of Readout Request/Data Requests to send before starting to read data (Default: 0)." << endl
		<< "    -q: Quiet mode (Don't print)" << endl
		<< "    -s: Stop on SERDES Error." << endl
		<< "    -e: Use DTC CFO Emulator instead of DTCLib's SoftwareCFO" << endl
		<< "    -t: Use DebugType flag (1st request gets ExternalDataWithFIFOReset, the rest get ExternalData)" << endl
		<< "    -T: Set DebugType flag for ALL requests (0, 1, or 2)" << endl
		<< "    -f: RAW Output file path" << endl;
	exit(0);
}

int
main(int	argc
	, char	*argv[])
{
	bool incrementTimestamp = true;
	bool checkSERDES = false;
	bool quiet = false;
	bool rawOutput = false;
	bool useCFOEmulator = false;
	std::string rawOutputFile = "/tmp/mu2eUtil.raw";
	unsigned delay = 0;
	unsigned number = 1;
	unsigned timestampOffset = 1;
	unsigned packetCount = 0;
	int requestsAhead = 0;
	string op = "";
	DTCLib::DTC_DebugType debugType = DTCLib::DTC_DebugType_SpecialSequence;
	bool stickyDebugType = true;
	int val = 0;

	for (int optind = 1; optind < argc; ++optind) {
		if (argv[optind][0] == '-') {
			switch (argv[optind][1]) {
			case 'i':
				incrementTimestamp = false;
				break;
			case 'd':
				delay = getOptionValue(&optind, &argv);
				break;
			case 'n':
				number = getOptionValue(&optind, &argv);
				break;
			case 'o':
				timestampOffset = getOptionValue(&optind, &argv);
				break;
			case 'c':
				packetCount = getOptionValue(&optind, &argv);
				break;
			case 'a':
				requestsAhead = getOptionValue(&optind, &argv);
				break;
			case 'q':
				quiet = true;
				break;
			case 's':
				checkSERDES = true;
				break;
			case 'e':
				useCFOEmulator = true;
				break;
			case 'f':
				rawOutput = true;
				rawOutputFile = getOptionString(&optind, &argv);
				break;
			case 't':
				debugType = DTCLib::DTC_DebugType_ExternalSerialWithReset;
				stickyDebugType = false;
				break;
			case 'T':
				val = getOptionValue(&optind, &argv);
				if (val == 0 || val == 1 || val == 2) {
					stickyDebugType = true;
					debugType = (DTCLib::DTC_DebugType)val;
					break;
				}
				cout << "Invalid Debug Type passed to -T!" << endl;
				printHelpMsg();
				break;
			default:
				cout << "Unknown option: " << argv[optind] << endl;
				printHelpMsg();
				break;
			case 'h':
				printHelpMsg();
				break;
			}
		}
		else {
			op = string(argv[optind]);
		}
	}

	string incrementStr = incrementTimestamp ? "true" : "false";
	string quietStr = quiet ? "true" : "false";
	string cfoStr = useCFOEmulator ? "true" : "false";
	string serdesStr = checkSERDES ? "true" : "false";
	string typeString = "Special Sequence";
	switch (debugType) {
	case DTC_DebugType_SpecialSequence:
		break;
	case DTC_DebugType_ExternalSerial:
		typeString = "External Serial";
		break;
	case DTC_DebugType_ExternalSerialWithReset:
		typeString = "External Serial w/ FIFO Reset";
		if (!stickyDebugType) typeString += ", will change to External Serial after first Request";
		break;
	}
	cout << "Options are: "
		<< "Operation: " << string(op)
		<< ", Num: " << number
		<< ", Delay: " << delay
		<< ", TS Offset: " << timestampOffset
		<< ", PacketCount: " << packetCount
		<< ", Requests Ahead of Reads: " << requestsAhead
		<< ", Use DTC CFO Emulator: " << cfoStr
		<< ", Increment TS: " << incrementStr
		<< ", Quiet Mode: " << quietStr
		<< ", Check SERDES Error Status: " << serdesStr
		<< ", Debug Type: " << typeString
		<< endl;

	if (op == "read")
	{
		cout << "Operation \"read\"" << endl;
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);
		DTC_DataHeaderPacket* packet = thisDTC->ReadNextDAQPacket();
		cout << packet->toJSON() << '\n';
		if (rawOutput) {
			std::ofstream outputStream;
			outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
			DTC_DataPacket rawPacket = packet->ConvertToDataPacket();
			for (int ii = 0; ii < 16; ++ii)
			{
				uint8_t word = rawPacket.GetWord(ii);
				outputStream.write((char*)&word, sizeof(uint8_t));
			}
			outputStream.close();
		}
	}
	else if (op == "read_data")
	{
		cout << "Operation \"read_data\"" << endl;
		mu2edev device;
		device.init();
		for (unsigned ii = 0; ii < number; ++ii)
		{
			mu2e_databuff_t *buffer = (mu2e_databuff_t*)new mu2e_databuff_t();
			int tmo_ms = 0;
			int sts = device.read_data(DTC_DMA_Engine_DAQ, (void**)buffer, tmo_ms);
			TRACE(1, "util - read for DAQ - ii=%u sts=%d %p", ii, sts, *buffer);
			if (rawOutput) {
				std::ofstream outputStream;
				outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
				for (int ii = 0; ii < sts; ++ii)
				{
					outputStream.write((char*)(buffer[ii]), sizeof(unsigned char));
				}
				outputStream.close();
			}
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "toggle_serdes")
	{
		cout << "Swapping SERDES Oscillator Clock" << endl;
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);
		thisDTC->ToggleSERDESOscillatorClock();
	}
	else if (op == "loopback")
	{
		cout << "Operation \"loopback\"" << endl;
		double totalIncTime = 0, totalSize = 0;
		auto startTime = std::chrono::high_resolution_clock::now();
		DTC *thisDTC = new DTC(DTC_SimMode_Loopback);
		mu2edev device = thisDTC->GetDevice();

		if (thisDTC->ReadSimMode() != DTC_SimMode_Loopback) {
			cout << "You must run this operation with DTCLIB_SIM_ENABLE unset or \"L\"!" << endl;
			exit(1);
		}
		unsigned ii = 0;
		for (; ii < number; ++ii)
		{
			uint64_t ts = timestampOffset + (incrementTimestamp ? ii : 0);
			DTC_DataHeaderPacket header(DTC_Ring_0, (uint16_t)0, DTC_DataStatus_Valid, DTC_Timestamp(ts));
			if (!quiet) std::cout << "Request: " << header.toJSON() << std::endl;
			thisDTC->WriteDMADAQPacket(header);
			bool returned = false;
			int count = 5;
			while (!returned && count > 0)
			{
				mu2e_databuff_t* buffer;
				int tmo_ms = 0xffffffff;
				auto startDTC = std::chrono::high_resolution_clock::now();
				device.release_all(DTC_DMA_Engine_DAQ);
				int sts = device.read_data(DTC_DMA_Engine_DAQ, (void**)&buffer, tmo_ms);
				auto endDTC = std::chrono::high_resolution_clock::now();
				totalIncTime += std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
					(endDTC - startDTC).count();
				count--;
				if (sts > 0)
				{
					void* readPtr = &(buffer[0]);
					uint16_t bufSize = static_cast<uint16_t>(*((uint64_t*)readPtr));
					totalSize += bufSize;
					DTC_DataHeaderPacket out = DTC_DataHeaderPacket(DTC_DataPacket(&(buffer[8])));;
					returned = out == header;
				}
				if (delay > 0) usleep(delay);
			}
			if (!returned) { break; }
			if (delay > 0) usleep(delay);
		}

		double aveRate = totalSize / totalIncTime / 1024;

		auto totalTime = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
			(std::chrono::high_resolution_clock::now() - startTime).count();
		std::cout << "STATS, " << std::dec << ii << " DataHeaders looped back:" << std::endl
			<< "(Out of " << number << " requested)" << std::endl
			<< "Total Elapsed Time: " << totalTime << " s." << std::endl
			<< "DTC::GetData Time: " << totalIncTime << " s." << std::endl
			<< "Total Data Size: " << totalSize / 1024 << " KB." << std::endl
			<< "Average Data Rate: " << aveRate << " KB/s." << std::endl;
	}
	else if (op == "buffer_test")
	{
		cout << "Operation \"buffer_test\"" << endl;
		double totalIncTime = 0, totalSize = 0;
		auto startTime = std::chrono::high_resolution_clock::now();
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);
		if (!thisDTC->ReadSERDESOscillatorClock()) { thisDTC->ToggleSERDESOscillatorClock(); } // We're going to 2.5Gbps for now    

		mu2edev device = thisDTC->GetDevice();
		if (thisDTC->ReadSimMode() != DTC_SimMode_Loopback) {
			DTCSoftwareCFO *cfo = new DTCSoftwareCFO(thisDTC, useCFOEmulator, packetCount, debugType, stickyDebugType, quiet, false);
			cfo->SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);
		}
		else {
			uint64_t ts = timestampOffset;
			DTC_DataHeaderPacket header(DTC_Ring_0, (uint16_t)0, DTC_DataStatus_Valid, DTC_Timestamp(ts));
			std::cout << "Request: " << header.toJSON() << std::endl;
			thisDTC->WriteDMADAQPacket(header);
		}
		double readoutRequestTime = thisDTC->GetDeviceTime();

		std::ofstream outputStream;
		if (rawOutput) outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);

		for (unsigned ii = 0; ii < number; ++ii)
		{
			cout << "Buffer Read " << ii << endl;
			mu2e_databuff_t* buffer;
			int tmo_ms = 0xffffffff;
			auto startDTC = std::chrono::high_resolution_clock::now();
			device.release_all(DTC_DMA_Engine_DAQ);
			int sts = device.read_data(DTC_DMA_Engine_DAQ, (void**)&buffer, tmo_ms);
			auto endDTC = std::chrono::high_resolution_clock::now();
			totalIncTime += std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
				(endDTC - startDTC).count();

			TRACE(1, "util - read for DAQ - ii=%u sts=%d %p", ii, sts, (void*)buffer);
			if (sts > 0) {
				void* readPtr = &(buffer[0]);
				uint16_t bufSize = static_cast<uint16_t>(*((uint64_t*)readPtr));
				totalSize += bufSize;
				readPtr = (uint8_t*)readPtr + 8;
				TRACE(1, "util - bufSize is %u", bufSize);
				for (unsigned line = 0; line < (unsigned)(ceil((bufSize - 8) / 16)); ++line)
				{
					cout << "0x" << hex << setw(5) << setfill('0') << line << "0: ";
					for (unsigned byte = 0; byte < 16; ++byte)
					{
						if ((line * 16) + byte < (bufSize - 8u)) {
							cout << setw(2) << (int)(((uint8_t*)buffer)[8 + (line * 16) + byte]) << " ";
							if (rawOutput) outputStream.write((char*)&(((uint8_t*)buffer)[8 + (line * 16) + byte]), sizeof(uint8_t));
						}
					}
					cout << endl;
				}
			}
			cout << endl << endl;
			if (delay > 0) usleep(delay);
		}
		if (rawOutput) outputStream.close();

		double aveRate = totalSize / totalIncTime / 1024;

		auto totalTime = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
			(std::chrono::high_resolution_clock::now() - startTime).count();
		std::cout << "STATS, "
			<< "Total Elapsed Time: " << totalTime << " s." << std::endl
			<< "Devive Time: " << totalIncTime << " s." << std::endl
			<< "Total Data Size: " << totalSize / 1024 << " KB." << std::endl
			<< "Average Data Rate: " << aveRate << " KB/s." << std::endl
			<< "Readout Request Time: " << readoutRequestTime << " s." << std::endl;
	}
	else if (op == "read_release")
	{
		mu2edev device;
		device.init();
		for (unsigned ii = 0; ii < number; ++ii)
		{
			void *buffer;
			int tmo_ms = 0;
			int stsRD = device.read_data(DTC_DMA_Engine_DAQ, &buffer, tmo_ms);
			int stsRL = device.read_release(DTC_DMA_Engine_DAQ, 1);
			TRACE(12, "util - release/read for DAQ and DCS ii=%u stsRD=%d stsRL=%d %p", ii, stsRD, stsRL, buffer);
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "HW")
	{
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);

		for (unsigned ii = 0; ii < number; ++ii)
		{
			uint64_t ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
			DTC_DataHeaderPacket header(DTC_Ring_0, (uint16_t)0, DTC_DataStatus_Valid, DTC_Timestamp(ts));
			std::cout << "Request: " << header.toJSON() << std::endl;
			thisDTC->WriteDMADAQPacket(header);
			thisDTC->SetFirstRead(true);
			DTC_DataHeaderPacket* packet = thisDTC->ReadNextDAQPacket();
			std::cout << "Reply:   " << packet->toJSON() << std::endl;
			if (rawOutput) {
				std::ofstream outputStream;
				outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
				DTC_DataPacket rawPacket = packet->ConvertToDataPacket();
				for (int ii = 0; ii < 16; ++ii)
				{
					uint8_t word = rawPacket.GetWord(ii);
					outputStream.write((char*)&word, sizeof(uint8_t));
				}
				outputStream.close();
			}
			if (delay > 0) usleep(delay);
		}
	}
	else if (op == "DTC")
	{
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);
		if (!thisDTC->ReadSERDESOscillatorClock()) { thisDTC->ToggleSERDESOscillatorClock(); } // We're going to 2.5Gbps for now    

		double totalIncTime = 0, totalSize = 0, totalDevTime = 0;
		auto startTime = std::chrono::high_resolution_clock::now();

		DTCSoftwareCFO *theCFO = new DTCSoftwareCFO(thisDTC, useCFOEmulator, packetCount, debugType, stickyDebugType, quiet);
		theCFO->SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);
		double readoutRequestTime = thisDTC->GetDeviceTime();
		thisDTC->ResetDeviceTime();

		unsigned ii = 0;
		int retries = 4;
		uint64_t expectedTS = timestampOffset;
		for (; ii < number; ++ii)
		{
			//if(delay > 0) usleep(delay);
			//uint64_t ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
			auto startDTC = std::chrono::high_resolution_clock::now();
			vector<void*> data = thisDTC->GetData(); //DTC_Timestamp(ts));
			auto endDTC = std::chrono::high_resolution_clock::now();
			totalIncTime += std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
				(endDTC - startDTC).count();

			totalDevTime += thisDTC->GetDeviceTime();
			thisDTC->ResetDeviceTime();

			if (data.size() > 0)
			{
				TRACE(19, "util_main %zu packets returned", data.size());
				if (!quiet) cout << data.size() << " packets returned\n";
				for (size_t i = 0; i < data.size(); ++i)
				{
					TRACE(19, "util_main constructing DataPacket:");
					DTC_DataPacket     test = DTC_DataPacket(data[i]);
					TRACE(19, test.toJSON().c_str());
					if (!quiet) cout << test.toJSON() << '\n'; // dumps whole databuff_t
					//printf("data@%p=0x%08x\n", data[i], *(uint32_t*)(data[i]));
					//DTC_DataHeaderPacket h1 = DTC_DataHeaderPacket(data[i]);
					//cout << h1.toJSON() << '\n';
					DTC_DataHeaderPacket h2 = DTC_DataHeaderPacket(test);
					if (expectedTS != h2.GetTimestamp().GetTimestamp(true))
					{
						cout << dec << h2.GetTimestamp().GetTimestamp(true) << " does not match expected timestamp of " << expectedTS << "!!!" << endl;
						expectedTS = h2.GetTimestamp().GetTimestamp(true) + (incrementTimestamp ? 1 : 0);
					}
					else {
						expectedTS += (incrementTimestamp ? 1 : 0);
					}
					TRACE(19, h2.toJSON().c_str());
					if (!quiet) {
						cout << h2.toJSON() << '\n';
						if (rawOutput) {
							std::ofstream outputStream;
							outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
							DTC_DataPacket rawPacket = h2.ConvertToDataPacket();
							for (int ii = 0; ii < 16; ++ii)
							{
								uint8_t word = rawPacket.GetWord(ii);
								outputStream.write((char*)&word, sizeof(uint8_t));
							}
							outputStream.close();
						}
						for (int jj = 0; jj < h2.GetPacketCount(); ++jj) {
							DTC_DataPacket packet = DTC_DataPacket(((uint8_t*)data[i]) + ((jj + 1) * 16));
							cout << "\t" << packet.toJSON() << endl;
							if (rawOutput) {
								std::ofstream outputStream;
								outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
								for (int ii = 0; ii < 16; ++ii)
								{
									uint8_t word = packet.GetWord(ii);
									outputStream.write((char*)&word, sizeof(uint8_t));
								}
								outputStream.close();
							}
						}
					}
					totalSize += 16 * (1 + h2.GetPacketCount());
				}
			}
			else
			{
				//TRACE_CNTL("modeM", 0L);
				if (!quiet) cout << "no data returned\n";
				//return (0);
				//break;
				usleep(100000);
				ii--;
				retries--;
				if (retries <= 0) break;
				continue;
			}
			retries = 4;

			if (checkSERDES) {
				auto disparity = thisDTC->ReadSERDESRXDisparityError(DTC_Ring_0);
				auto cnit = thisDTC->ReadSERDESRXCharacterNotInTableError(DTC_Ring_0);
				auto rxBufferStatus = thisDTC->ReadSERDESRXBufferStatus(DTC_Ring_0);
				bool eyescan = thisDTC->ReadSERDESEyescanError(DTC_Ring_0);
				if (eyescan) {
					//TRACE_CNTL("modeM", 0L);
					cout << "SERDES Eyescan Error Detected" << endl;
					//return 0;
					break;
				}
				if ((int)rxBufferStatus > 2) {
					//TRACE_CNTL("modeM", 0L);
					cout << "Bad Buffer status detected: " << rxBufferStatus << endl;
					//return 0;
					break;
				}
				if (cnit.GetData()[0] || cnit.GetData()[1]) {
					//TRACE_CNTL("modeM", 0L);
					cout << "Character Not In Table Error detected" << endl;
					//return 0;
					break;
				}
				if (disparity.GetData()[0] || disparity.GetData()[1]) {
					//TRACE_CNTL("modeM", 0L);
					cout << "Disparity Error Detected" << endl;
					//return 0;
					break;
				}
			}
		}

		double aveRate = totalSize / totalDevTime / 1024;

		auto totalTime = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1> >>
			(std::chrono::high_resolution_clock::now() - startTime).count();
		std::cout << "STATS, " << ii << " DataBlocks processed:" << std::endl
			<< "Total Elapsed Time: " << totalTime << " s." << std::endl
			<< "DTC::GetData Time: " << totalIncTime << " s." << std::endl
			<< "Total Data Size: " << totalSize / 1024 << " KB." << std::endl
			<< "Average Data Rate: " << aveRate << " KB/s." << std::endl
			<< "Readout Request Time: " << readoutRequestTime << " s." << std::endl
			<< "Total Device Time: " << totalDevTime << " s." << std::endl;
	}
	else// if (argc > 1 && strcmp(argv[1],"get")==0)
	{
		DTC *thisDTC = new DTC(DTC_SimMode_NoCFO);

		DTCSoftwareCFO *theCFO = new DTCSoftwareCFO(thisDTC, useCFOEmulator, packetCount, debugType, stickyDebugType, quiet);
		theCFO->SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);

		for (unsigned ii = 0; ii < number; ++ii)
		{
			//if(delay > 0) usleep(delay);
			//uint64_t ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
			vector<void*> data = thisDTC->GetData(); // DTC_Timestamp(ts));

			if (data.size() > 0)
			{
				//cout << data.size() << " packets returned\n";
				for (size_t i = 0; i < data.size(); ++i)
				{
					TRACE(19, "DTC::GetJSONData constructing DataPacket:");
					DTC_DataPacket     test = DTC_DataPacket(data[i]);
					//cout << test.toJSON() << '\n'; // dumps whole databuff_t
					//printf("data@%p=0x%08x\n", data[i], *(uint32_t*)(data[i]));
					//DTC_DataHeaderPacket h1 = DTC_DataHeaderPacket(data[i]);
					//cout << h1.toJSON() << '\n';
					DTC_DataHeaderPacket h2 = DTC_DataHeaderPacket(test);
					if (rawOutput) {
						std::ofstream outputStream;
						outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);
						for (int ii = 0; ii < 16; ++ii)
						{
							uint8_t word = test.GetWord(ii);
							outputStream.write((char*)&word, sizeof(uint8_t));
						}
						outputStream.close();
					}
					//cout << h2.toJSON() << '\n';
					// for (int jj = 0; jj < h2.GetPacketCount(); ++jj) {
					//    cout << "\t" << DTC_DataPacket(((uint8_t*)data[i]) + ((jj + 1) * 16)).toJSON() << endl;
					//}
				}
			}
			else
			{
				TRACE_CNTL("modeM", 0L);
				cout << "no data returned\n";
				return (0);
			}
		}
	}
	return (0);
}   // main
