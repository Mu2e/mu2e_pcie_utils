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

#include "trace.h"

#include "cfoInterfaceLib/CFO_Compiler.hh"
#include "cfoInterfaceLib/CFO_Registers.h"

using namespace CFOLib;

bool rawOutput = false;
std::string rawOutputFile = "/tmp/cfoUtil.raw";
std::string inputFile = "/tmp/cfoUtil.raw";
bool compileInputFile = false;
std::ofstream outputStream;
unsigned targetFrequency = 166666667;
unsigned clockSpeed = 40000000;

int dtc = -1;
std::string op = "";

unsigned getOptionValue(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0') {
		(*index)++;
		unsigned ret = strtoul((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}
	auto offset = 2;
	if (arg[2] == '=') {
		offset = 3;
	}

	return strtoul(&arg[offset], nullptr, 0);
}
unsigned long long getOptionValueLong(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0') {
		(*index)++;
		unsigned long long ret = strtoull((*argv)[*index], nullptr, 0);
		if (ret == 0 && (*argv)[*index][0] != '0')  // No option given
		{
			(*index)--;
		}
		return ret;
	}
	auto offset = 2;
	if (arg[2] == '=') {
		offset = 3;
	}

	return strtoull(&arg[offset], nullptr, 0);
}

std::string getOptionString(int* index, char** argv[])
{
	auto arg = (*argv)[*index];
	if (arg[2] == '\0') {
		(*index)++;
		return std::string((*argv)[*index]);
	}
	auto offset = 2;
	if (arg[2] == '=') {
		offset = 3;
	}

	return std::string(&arg[offset]);
}

unsigned getLongOptionValue(int* index, char** argv[])
{
	auto arg = std::string((*argv)[*index]);
	auto pos = arg.find('=');

	if (pos == std::string::npos) {
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

	if (pos == std::string::npos) {
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

	if (pos == std::string::npos) {
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

	if (arg.find('=') == std::string::npos) {
		return std::string((*argv)[++(*index)]);
	}
	else
	{
		return arg.substr(arg.find('='));
	}
}

void printHelpMsg()
{
	std::cout << "Usage: cfoUtil [options] [write_program,program_clock,dma_info]" << std::endl;
	std::cout
		<< "Options are:" << std::endl
		<< "    -h, --help: This message." << std::endl
		<< "    -f: RAW Output file path" << std::endl
		<< "    -F: Frequency to program (in Hz, sorry...Default 166666667 Hz)" << std::endl
		<< "    -p: CFO program to write (Default: /tmp/cfoUtil.raw)" << std::endl
		<< "    -C: CFO program needs to be compiled (i.e. not bitfile). If specified with -f, will not write compiled "
		   "program to CFO"
		<< std::endl
		<< "    -c: Clock speed for CFO (default 40000000)" << std::endl
		<< "    --cfo: Use cfo <num> (Defaults to DTCLIB_DTC if set, 0 otherwise, see ls /dev/mu2e* for available CFOs)"
		<< std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	for (auto optind = 1; optind < argc; ++optind) {
		if (argv[optind][0] == '-') {
			switch (argv[optind][1])
			{
				case 'f':
					rawOutput = true;
					rawOutputFile = getOptionString(&optind, &argv);
					break;
				case 'F':
					targetFrequency = getOptionValue(&optind, &argv);
					break;
				case 'c':
					clockSpeed = getOptionValue(&optind, &argv);
					break;
				case 'C':
					compileInputFile = true;
					break;
				case 'p':
					inputFile = getOptionString(&optind, &argv);
					break;
				case '-':  // Long option
				{
					auto option = getLongOptionOption(&optind, &argv);
					if (option == "--cfo") {
						dtc = getLongOptionValue(&optind, &argv);
					}
					else if (option == "--help")
					{
						printHelpMsg();
					}
					break;
				}
				default:
					std::cout << "Unknown option: " << argv[optind] << std::endl;
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

	std::cout.setf(std::ios_base::boolalpha);
	std::cout << "Options are: "
			  << "Operation: " << std::string(op) << ", CFO: " << dtc;
	if (rawOutput) {
		std::cout << ", Raw output file: " << rawOutputFile;
	}
	std::cout << std::endl;
	if (rawOutput) outputStream.open(rawOutputFile, std::ios::out | std::ios::app | std::ios::binary);

	if (op == "write_program") {
		std::stringstream f;
		std::ifstream ifstr(inputFile);
		while (!ifstr.eof()) {
			std::string line;
			ifstr >> line;
			f << line;
		}
		std::string input = f.str();
		std::deque<char> inputBytes;
		mu2e_databuff_t inputData;

		if (compileInputFile) {
			auto compiler = new CFO_Compiler(clockSpeed);
			inputBytes = compiler->processFile(CFO_Source_File(input));

			if (rawOutput) {
				for (auto ch : inputBytes) {
					outputStream << ch;
				}
			}
			else
			{
				size_t offset = 8;
				auto inputSize = input.size();
				//*reinterpret_cast<uint64_t*>(inputData) = inputBytes.size();
				memcpy(&inputData[0], &inputSize, sizeof(uint64_t));
				for (auto ch : inputBytes) {
					inputData[offset++] = ch;
				}
				return 0;
			}
		}
		else
		{
			auto inputSize = input.size();
			//*reinterpret_cast<uint64_t*>(inputData) = input.size();
			memcpy(&inputData[0], &inputSize, sizeof(uint64_t));
			memcpy(&inputData[8], &input[0], input.size());
		}

		mu2edev dev;
		dev.init(DTC_SimMode_NoCFO, dtc);
		dev.write_data(DTC_DMA_Engine_DAQ, inputData, sizeof(inputData));
	}
	else if (op == "program_clock")
	{
		auto thisDTC = new CFO_Registers(DTC_SimMode_NoCFO, dtc);
		thisDTC->SetNewOscillatorFrequency(targetFrequency);
		delete thisDTC;
	}
	else if (op == "dma_info")
	{
		if (dtc == -1) {
			auto dtcE = getenv("DTCLIB_DTC");
			if (dtcE != nullptr) {
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
		std::cout << "Unrecognized operation: " << op << std::endl;
		printHelpMsg();
	}

	if (rawOutput) outputStream.close();
	return 0;
}  // main
