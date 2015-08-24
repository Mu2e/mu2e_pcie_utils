// This file (util_main.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 13, 2015. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <cstdio>		// printf
#include <cstdlib>		// strtoul
#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
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

void printHelpMsg() {
    cout << "Usage: mu2eUtil [options] [read,read_data,buffer_test,read_release,HW,DTC]" << endl;
    cout << "Options are:" << endl
        << "    -h: This message." << endl
        << "    -n: Number of times to repeat test. (Default: 1)" << endl
        << "    -p: Pause after sending a packet." << endl
        << "    -o: Starting Timestamp offest. (Default: 1)." << endl
        << "    -i: Do not increment Timestamps." << endl
        << "    -d: Delay between tests, in us (Default: 0)." << endl
        << "    -c: Number of Debug Packets to request (Default: 0)." << endl
        << "    -a: Number of Readout Request/Data Requests to send before starting to read data (Default: 2)." << endl
        << "    -q: Quiet mode (Don't print)" << endl
        << "    -s: Stop on SERDES Error." << endl;
    exit(0);
}

int
main(int	argc
, char	*argv[])
{
    bool pause = false;
    bool incrementTimestamp = true;
    bool checkSERDES = false;
    bool quiet = false;
    unsigned delay = 0;
    unsigned number = 1;
    unsigned timestampOffset = 1;
    unsigned packetCount = 0;
    int requestsAhead = 2;
    string op = "";

    for (int optind = 1; optind < argc; ++optind) {
        if (argv[optind][0] == '-') {
            switch (argv[optind][1]) {
            case 'p':
                pause = true;
                break;
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

    string pauseStr = pause ? "true" : "false";
    string incrementStr = incrementTimestamp ? "true" : "false";
    string quietStr = quiet ? "true" : "false";
    cout << "Options are: "
        << "Operation: " << string(op)
        << ", Num: " << number
        << ", Delay: " << delay
        << ", TS Offset: " << timestampOffset
        << ", PacketCount: " << packetCount
        << ", Requests Ahead of Reads: " << requestsAhead
        << ", Pause: " << pauseStr
        << ", Increment TS: " << incrementStr
        << ", Quiet Mode: " << quietStr
        << endl;

    if (op == "read")
    {
        cout << "Operation \"read\"" << endl;
        DTC *thisDTC = new DTC(DTC_SimMode_Hardware);
        DTC_DataHeaderPacket* packet = thisDTC->ReadNextDAQPacket();
        cout << packet->toJSON() << '\n';
    }
    else if (op == "read_data")
    {
        cout << "Operation \"read_data\"" << endl;
        mu2edev device;
        device.init();
        for (unsigned ii = 0; ii < number; ++ii)
        {
            void *buffer;
            int tmo_ms = 0;
            int sts = device.read_data(DTC_DMA_Engine_DAQ, &buffer, tmo_ms);
            TRACE(1, "util - read for DAQ - ii=%u sts=%d %p", ii, sts, buffer);
            if (delay > 0) usleep(delay);
        }
    }
    else if (op == "buffer_test")
    {
        cout << "Operation \"buffer_test\"" << endl;
        DTC *thisDTC = new DTC(DTC_SimMode_Hardware);
        thisDTC->EnableRing(DTC_Ring_0, DTC_RingEnableMode(true, true, false), DTC_ROC_0);
        thisDTC->SetInternalSystemClock();
        thisDTC->DisableTiming();
        thisDTC->SetMaxROCNumber(DTC_Ring_0, DTC_ROC_0);
        if (!thisDTC->ReadSERDESOscillatorClock()) { thisDTC->ToggleSERDESOscillatorClock(); } // We're going to 2.5Gbps for now    

        mu2edev device = thisDTC->GetDevice();
        DTCSoftwareCFO cfo(thisDTC, packetCount, quiet, false);
        cfo.SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);

        for (unsigned ii = 0; ii < number; ++ii)
        {
            cout << "Buffer Read " << ii << endl;
            mu2e_databuff_t* buffer;
            int tmo_ms = 0;
            device.release_all(DTC_DMA_Engine_DAQ);
            int sts = device.read_data(DTC_DMA_Engine_DAQ, (void**)&buffer, tmo_ms);
            TRACE(1, "util - read for DAQ - ii=%u sts=%d %p", ii, sts, (void*)buffer);
            if (sts > 0) {
                void* readPtr = &(buffer[0]);
                uint16_t bufSize = static_cast<uint16_t>(*((uint64_t*)readPtr));
                readPtr = (uint8_t*)readPtr + 8;
                TRACE(1, "util - bufSize is %u", bufSize);
                for (unsigned line = 0; line < (unsigned)(((bufSize -8) / 16)); ++line)
                {
                    cout << "0x" << hex << setw(5) << setfill('0') << line << "0: ";
                    for (unsigned byte = 0; byte < 16; ++byte)
                    {
                        if ((line * 16) + byte < (bufSize - 8u)) cout << setw(2) << (int)(((uint8_t*)buffer)[8 + (line * 16) + byte]) << " ";
                    }
                    cout << endl;
                }
            }
            cout << endl << endl;
            if (delay > 0) usleep(delay);
        }
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
        DTC *thisDTC = new DTC(DTC_SimMode_Hardware);
        thisDTC->EnableRing(DTC_Ring_0, DTC_RingEnableMode(true, true, false), DTC_ROC_0);
        thisDTC->SetInternalSystemClock();
        thisDTC->DisableTiming();
        thisDTC->SetMaxROCNumber(DTC_Ring_0, DTC_ROC_0);

        for (unsigned ii = 0; ii < number; ++ii)
        {
            uint64_t ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
            DTC_DataHeaderPacket header(DTC_Ring_0, (uint16_t)0, DTC_DataStatus_Valid, DTC_Timestamp(ts));
            std::cout << "Request: " << header.toJSON() << std::endl;
            thisDTC->WriteDMADAQPacket(header);
            thisDTC->SetFirstRead(true);
            std::cout << "Reply:   " << thisDTC->ReadNextDAQPacket()->toJSON() << std::endl;
            if (delay > 0) usleep(delay);
        }
    }
    else if (op == "DTC")
    {
        DTC *thisDTC = new DTC(DTC_SimMode_Hardware);
        thisDTC->EnableRing(DTC_Ring_0, DTC_RingEnableMode(true, true, false), DTC_ROC_0);
        thisDTC->SetInternalSystemClock();
        thisDTC->DisableTiming();
        thisDTC->SetMaxROCNumber(DTC_Ring_0, DTC_ROC_0);
        if (!thisDTC->ReadSERDESOscillatorClock()) { thisDTC->ToggleSERDESOscillatorClock(); } // We're going to 2.5Gbps for now    

        double totalIncTime = 0, totalSize = 0, totalDevTime = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        DTCSoftwareCFO theCFO(thisDTC, packetCount, quiet);
        theCFO.SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);
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

            if (pause) {
                std::cout << "GetData Called. Press any key." << std::endl;
                std::string dummy;
                getline(std::cin, dummy);
            }

            if (data.size() > 0)
            {
                TRACE(19, "util_main %lu packets returned", data.size());
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
                        for (int jj = 0; jj < h2.GetPacketCount(); ++jj) {
                            cout << "\t" << DTC_DataPacket(((uint8_t*)data[i]) + ((jj + 1) * 16)).toJSON() << endl;
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
        DTC *thisDTC = new DTC(DTC_SimMode_Hardware);

        DTCSoftwareCFO theCFO(thisDTC, packetCount, quiet);
        theCFO.SendRequestsForRange(number, DTC_Timestamp(timestampOffset), incrementTimestamp, delay, requestsAhead);

        for (unsigned ii = 0; ii < number; ++ii)
        {
            //if(delay > 0) usleep(delay);
            //uint64_t ts = incrementTimestamp ? ii + timestampOffset : timestampOffset;
            vector<void*> data = thisDTC->GetData(); // DTC_Timestamp(ts));
            if (pause) {
                std::cout << "GetData Called. Press any key." << std::endl;
                std::string dummy;
                getline(std::cin, dummy);
            }

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
