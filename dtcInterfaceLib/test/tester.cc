#include <unistd.h>  // usleep
#include <chrono>
#include <iostream>

#include "dtcInterfaceLib/DTC.h"
#include "dtcInterfaceLib/DTCSoftwareCFO.h"

#include "TRACE/tracemf.h"

#include "fragmentTester.h"

using namespace DTCLib;

void usage()
{
	std::cout << "Usage: tester [loops = 1000] [simMode = 1] [simFile = \"\"]" << std::endl;
	exit(1);
}

int main(int argc, char* argv[])
{
	auto loops = 1000;
	auto modeint = 1;
	std::string simFile = "";
	auto badarg = false;
	if (argc > 1)
	{
		auto tmp = atoi(argv[1]);
		if (tmp > 0)
			loops = tmp;
		else
			badarg = true;
	}
	if (argc > 2)
	{
		auto tmp = atoi(argv[2]);
		if (tmp > 0)
			modeint = tmp;
		else
			badarg = true;
	}
	if (argc > 3)
	{
		simFile = std::string(argv[3]);
	}
	if (argc > 4) badarg = true;
	if (badarg) usage();  // Exits.

	TRACE(1, "simFile is %s", simFile.c_str());
	auto mode = DTC_SimModeConverter::ConvertToSimMode(std::to_string(modeint));
	auto thisDTC = new DTC(mode, 0);
	if (simFile.size() > 0)
	{
		thisDTC->WriteSimFileToDTC(simFile, true, true);
	}
	TRACE(1, "thisDTC->ReadSimMode: %i", thisDTC->ReadSimMode());
	auto theCFO = new DTCSoftwareCFO(thisDTC, true);
	long loopCounter = 0;
	long count = 0;
	typedef uint8_t packet_t[16];

	while (loopCounter < loops)
	{
		TRACE(1, "mu2eReceiver::getNext: Starting CFO thread");
		uint64_t z = 0;
		DTC_EventWindowTag zero(z);
		TRACE(1, "Sending requests for %i timestamps, starting at %lu", BLOCK_COUNT_MAX, BLOCK_COUNT_MAX * loopCounter);
		theCFO->SendRequestsForRange(BLOCK_COUNT_MAX, DTC_EventWindowTag(BLOCK_COUNT_MAX * loopCounter));

		fragmentTester newfrag(BLOCK_COUNT_MAX * sizeof(packet_t) * 2);

		DTC_EventWindowTag expected_timestamp;
		auto firstLoop = true;

		// Get data from DTCReceiver
		TRACE(1, "mu2eReceiver::getNext: Starting DTCFragment Loop");
		while (newfrag.hdr_block_count() < BLOCK_COUNT_MAX)
		{
			TRACE(1, "Getting DTC Data");
			std::vector<std::unique_ptr<DTC_Event>> data;
			auto retryCount = 5;
			while (data.size() == 0 && retryCount >= 0)
			{
				try
				{
					// TRACE(4, "Calling theInterface->GetData(zero)");
					data = thisDTC->GetData(zero);
					// TRACE(4, "Done calling theInterface->GetData(zero)");
				}
				catch (std::exception const& ex)
				{
					std::cerr << ex.what() << std::endl;
				}
				retryCount--;
				// if (data.size() == 0) { usleep(10000); }
			}
			if (retryCount < 0 && data.size() == 0)
			{
				TRACE(1, "Retry count exceeded. Something is very wrong indeed");
				std::cout << "Had an error with block " << newfrag.hdr_block_count() << " of event " << loopCounter
						  << std::endl;
				break;
			}

			auto first = DTC_DataHeaderPacket(DTC_DataPacket(data[0].blockPointer));
			auto ts = first.GetEventWindowTag();
			if (firstLoop)
			{
				expected_timestamp = ts;
				firstLoop = false;
			}
			if (ts != expected_timestamp)
			{
				std::cerr << "WRONG TIMESTAMP DETECTED: 0x" << std::hex << ts.GetEventWindowTag(true) << " (expected: 0x"
						  << expected_timestamp.GetEventWindowTag(true) << ")" << std::endl;
			}
			expected_timestamp = ts + 1;
			// int packetCount = first.GetPacketCount() + 1;
			// TRACE(1, "There are %lu data blocks in timestamp %lu. Packet count of first data block: %i", data.size(),
			// ts.GetEventWindowTag(true), packetCount);

			size_t totalSize = 0;

			for (size_t i = 0; i < data.size(); ++i)
			{
				totalSize += data[i].byteSize;
			}

			auto diff = static_cast<int64_t>(totalSize + newfrag.dataSize()) - newfrag.fragSize();
			TRACE(4, "diff=%lli, totalSize=%zu, dataSize=%zu, fragSize=%zu", (long long)diff, totalSize, newfrag.dataSize(),
				  newfrag.fragSize());
			if (diff > 0)
			{
				auto currSize = newfrag.fragSize();
				auto remaining = 1 - newfrag.hdr_block_count() / static_cast<double>(BLOCK_COUNT_MAX);
				auto newSize = static_cast<size_t>(currSize * remaining);
				TRACE(1, "mu2eReceiver::getNext: %zu + %zu > %zu, allocating space for %zu more bytes", totalSize,
					  newfrag.dataSize(), newfrag.fragSize(), newSize + diff);
				newfrag.addSpace(static_cast<size_t>(diff + newSize));
			}

			TRACE(3, "Copying DTC packets into Mu2eFragment");
			auto offset = newfrag.dataBegin() + newfrag.dataSize();
			size_t intraBlockOffset = 0;
			for (size_t i = 0; i < data.size(); ++i)
			{
				TRACE(4, "Copying data from %p to %p (sz=%zu)", data[i].blockPointer,
					  reinterpret_cast<void*>(offset + intraBlockOffset), data[i].byteSize);
				memcpy(reinterpret_cast<void*>(offset + intraBlockOffset), data[i].blockPointer, data[i].byteSize);
				intraBlockOffset += data[i].byteSize;
			}

			TRACE(3, "Ending SubEvt %zu", newfrag.hdr_block_count());
			newfrag.endSubEvt(intraBlockOffset);
		}

		loopCounter++;
		count += newfrag.hdr_block_count();
		// auto file = fopen((std::string("tester_") + std::to_string(loopCounter) + ".bin").c_str(),"w+");
		// fwrite((void*)newfrag.dataBegin(), sizeof(uint8_t), newfrag.fragSize(), file);
		std::cout << "Event: " << loopCounter << ": " << newfrag.hdr_block_count() << " timestamps. (" << count << " total)"
				  << std::endl;
	}

	delete theCFO;
	delete thisDTC;

	return 0;
}
