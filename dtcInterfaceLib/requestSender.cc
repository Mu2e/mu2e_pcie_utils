#include <unistd.h>  // usleep
#include <cstdio>    // printf
#include <cstdlib>   // strtoul
#include <iostream>
#include <string>

#include "DTCSoftwareCFO.h"

#include "TRACE/tracemf.h"

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

void printHelpMsg()
{
	std::cout << "Usage: requestSender [options]" << std::endl;
	std::cout << "Options are:" << std::endl
			  << "    -h: This message." << std::endl
			  << "    -n: Number of times to repeat test. (Default: 1)" << std::endl
			  << "    -o: Starting Timestamp offest. (Default: 1)." << std::endl
			  << "    -i: Do not increment Timestamps." << std::endl
			  << "    -d: Delay between tests, in us (Default: 0)." << std::endl
			  << "    -c: Number of Debug Packets to request (Default: 0)." << std::endl
			  << "    -q: Quiet mode (Don't print)" << std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	DTCLib::DTC theDTC;
	DTCLib::DTCSoftwareCFO theEmulator(&theDTC, false);

	auto incrementTimestamp = true;
	auto quiet = false;
	unsigned delay = 0;
	unsigned number = 1;
	unsigned timestampOffset = 1;
	unsigned packetCount = 0;

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
				case 'n':
					number = getOptionValue(&optind, &argv);
					break;
				case 'o':
					timestampOffset = getOptionValue(&optind, &argv);
					break;
				case 'c':
					packetCount = getOptionValue(&optind, &argv);
					break;
				case 'q':
					quiet = true;
					break;
				default:
					std::cout << "Unknown option: " << argv[optind] << std::endl;
					printHelpMsg();
					break;
				case 'h':
					printHelpMsg();
					break;
			}
		}
	}

	std::string incrementStr = incrementTimestamp ? "true" : "false";
	std::string quietStr = quiet ? "true" : "false";
	std::cout << "Options are: "
			  << "Num: " << number << ", Delay: " << delay << ", TS Offset: " << timestampOffset
			  << ", PacketCount: " << packetCount << ", Increment TS: " << incrementStr << ", Quiet Mode: " << quietStr
			  << std::endl;

	theEmulator.setQuiet(quiet);
	theEmulator.setDebugPacketCount(packetCount);

	if (number > 1)
	{
		theEmulator.SendRequestsForRange(number, DTCLib::DTC_Timestamp(timestampOffset), incrementTimestamp, delay);
	}
	else
	{
		theEmulator.SendRequestForTimestamp(DTCLib::DTC_Timestamp(timestampOffset));
	}
}
