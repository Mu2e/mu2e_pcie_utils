// This file (mu2edev.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb 13, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

/*
 *    make mu2edev.o CFLAGS='-g -Wall -std=c++0x'
 */

#define TRACE_NAME "MU2EDEV"
#ifndef _WIN32
# include <trace.h>
#else
# ifndef TRACE
#  include <stdio.h>
#  ifdef _DEBUG
#   define TRACE(lvl,...) printf(__VA_ARGS__); printf("\n")
#  else
#   define TRACE(...)
#  endif
# endif
# pragma warning(disable: 4351)
#endif
#include "mu2esim.hh"
#include <ctime>
#include <vector>
#include <forward_list>
#include <iostream>
#include <algorithm>
#include <cmath>

mu2esim::mu2esim()
    : hwIdx_()
    , swIdx_()
    , dmaData_()
    , mode_(DTCLib::DTC_SimMode_Disabled)
{
#ifndef _WIN32  
    //TRACE_CNTL( "lvlmskM", 0x3 );
    //TRACE_CNTL( "lvlmskS", 0x3 );
#endif
    hwIdx_[0] = 0;
    hwIdx_[1] = 0;
    swIdx_[0] = 0;
    swIdx_[1] = 0;
    for (int ii = 0; ii < SIM_BUFFCOUNT; ++ii) {
        dmaData_[0][ii] = (mu2e_databuff_t*)new mu2e_databuff_t();
        dmaData_[1][ii] = (mu2e_databuff_t*)new mu2e_databuff_t();
        buffSize_[0][ii] = 0;
        buffSize_[1][ii] = 0;
    }
    release_all(0);
    release_all(1);
    for (int ring = 0; ring < 6; ++ring)
    {
        for (int roc = 0; roc < 6; ++roc)
        {
            dcsRequestReceived_[ring][roc] = false;
            simIndex_[ring][roc] = 0;
            dcsRequest_[ring][roc] = DTCLib::DTC_DCSRequestPacket((DTCLib::DTC_Ring_ID)ring, (DTCLib::DTC_ROC_ID)roc);
        }
    }
}

mu2esim::~mu2esim()
{
    for (unsigned ii = 0; ii < MU2E_MAX_CHANNELS; ++ii) {
        for (unsigned jj = 0; jj < SIM_BUFFCOUNT; ++jj) {
            delete[] dmaData_[ii][jj];
        }
    }
}

int mu2esim::init(DTCLib::DTC_SimMode mode)
{
    TRACE(17, "mu2e Simulator::init");
    mode_ = mode;

    TRACE(17, "mu2esim::init Initializing registers");
    // Set initial register values...
    registers_[0x9000] = 0x00006363; // v99.99
    registers_[0x9004] = 0x53494D44; // SIMD in ASCII
    registers_[0x900C] = 0x00000010; // Send
    registers_[0x9010] = 0x00000040; // Recieve
    registers_[0x9014] = 0x00000100; // SPayload
    registers_[0x9018] = 0x00000400; // RPayload
    registers_[0x9100] = 0x40000003; // Clear latched errors, System Clock, Timing Enable
    registers_[0x9104] = 0x80000010; //Default value from HWUG
    registers_[0x9108] = 0x00049249; // SERDES Loopback PCS Near-End
    registers_[0x9168] = 0x00049249;
    registers_[0x910C] = 0x2; // Initialization Complete, no IIC Error
    registers_[0x9110] = 0x3F;        // ROC Emulators enabled (of course!)
    registers_[0x9114] = 0x3F3F;       // All rings Tx/Rx enabled, CFO and timing disabled
    registers_[0x9118] = 0x0;        // No SERDES Reset
    registers_[0x911C] = 0x0;        // No SERDES Disparity Error
    registers_[0x9120] = 0x0;        // No SERDES CNIT Error
    registers_[0x9124] = 0x0;        // No SERDES Unlock Error
    registers_[0x9128] = 0x7F;       // SERDES PLL Locked
    registers_[0x912C] = 0x0;        // SERDES TX Buffer Status Normal
    registers_[0x9130] = 0x0;        // SERDES RX Buffer Staus Nominal
    registers_[0x9134] = 0x0;        // SERDES RX Status Nominal
    registers_[0x9138] = 0x7F;       // SERDES Resets Done
    registers_[0x913C] = 0x0;        // No Eyescan Error
    registers_[0x9140] = 0x7F;       // RX CDR Locked
    registers_[0x9144] = 0x800;      // DMA Timeout Preset
    registers_[0x9148] = 0x200000;   // ROC Timeout Preset
    registers_[0x914C] = 0x0;        // ROC Timeout Error
    registers_[0x9150] = 0x0;        // Receive Packet Error
    registers_[0x9180] = 0x0;        // Timestamp preset to 0
    registers_[0x9184] = 0x0;
    registers_[0x9188] = 0x00002000; // Data pending timeout preset
    registers_[0x918C] = 0x1;          // NUMROCs 0 for all rings,except Ring 0 which has 1
    registers_[0x9190] = 0x0;  // NO FIFO Full flags
    registers_[0x9194] = 0x0;
    registers_[0x9198] = 0x0;
    registers_[0x9204] = 0x0010;     // Packet Size Bytes
    registers_[0x91A4] = 0x1;        // FPGA PROM Ready
    registers_[0x9404] = 0x1;
    registers_[0x9408] = 0x0;        // FPGA Core Access OK

    TRACE(17, "mu2esim::init Initializing DMA State Objects");
    // Set DMA State
    dmaState_[0][0].BDerrs = 0;
    dmaState_[0][0].BDSerrs = 0;
    dmaState_[0][0].BDs = 399;
    dmaState_[0][0].Buffers = 4;
    dmaState_[0][0].Engine = 0;
    dmaState_[0][0].IntEnab = 0;
    dmaState_[0][0].MaxPktSize = 0x100000;
    dmaState_[0][0].MinPktSize = 0x40;
    dmaState_[0][0].TestMode = 0;

    dmaState_[0][1].BDerrs = 0;
    dmaState_[0][1].BDSerrs = 0;
    dmaState_[0][1].BDs = 399;
    dmaState_[0][1].Buffers = 4;
    dmaState_[0][1].Engine = 0x20;
    dmaState_[0][1].IntEnab = 0;
    dmaState_[0][1].MaxPktSize = 0x100000;
    dmaState_[0][1].MinPktSize = 0x40;
    dmaState_[0][1].TestMode = 0;

    dmaState_[1][0].BDerrs = 0;
    dmaState_[1][0].BDSerrs = 0;
    dmaState_[1][0].BDs = 399;
    dmaState_[1][0].Buffers = 4;
    dmaState_[1][0].Engine = 1;
    dmaState_[1][0].IntEnab = 0;
    dmaState_[1][0].MaxPktSize = 0x100000;
    dmaState_[1][0].MinPktSize = 0x40;
    dmaState_[1][0].TestMode = 0;

    dmaState_[1][1].BDerrs = 0;
    dmaState_[1][1].BDSerrs = 0;
    dmaState_[1][1].BDs = 399;
    dmaState_[1][1].Buffers = 4;
    dmaState_[1][1].Engine = 0x21;
    dmaState_[1][1].IntEnab = 0;
    dmaState_[1][1].MaxPktSize = 0x100000;
    dmaState_[1][1].MinPktSize = 0x40;
    dmaState_[1][1].TestMode = 0;

    TRACE(17, "mu2esim::init Initializing PCIe State Object");
    // Set PCIe State
    pcieState_.VendorId = 4334;
    pcieState_.DeviceId = 28738;
    pcieState_.LinkState = true;
    pcieState_.LinkSpeed = 5;
    pcieState_.LinkWidth = 4;
    pcieState_.IntMode = 0;
    pcieState_.MPS = 256;
    pcieState_.MRRS = 512;
    pcieState_.Version = 0x53494D44;
    pcieState_.InitFCCplD = 0;
    pcieState_.InitFCCplH = 0;
    pcieState_.InitFCNPD = 16;
    pcieState_.InitFCNPH = 124;
    pcieState_.InitFCPD = 552;
    pcieState_.InitFCPH = 112;

    // Test State
    testStarted_ = false;
    testState_.Engine = 0;
    testState_.TestMode = 0;


    TRACE(17, "mu2esim::init finished");
    return (0);
}

/*****************************
   read_data
   returns number of bytes read; negative value indicates an error
   */
int mu2esim::read_data(int chn, void **buffer, int tmo_ms)
{
    if (delta_(chn, C2S) == 0) {
        clearBuffer_(chn, false);

        size_t currentOffset = 8;
        buffSize_[chn][swIdx_[chn]] = currentOffset;

        if (chn == 0)
        {
            std::set<uint64_t> activeTimestamps;
            rrMutex_.lock();
            for (int ring = 0; ring <= DTCLib::DTC_Ring_5; ++ring)
            {
                if (readoutRequestReceived_[ring].size() > 0) {
                    bool active = true;
                    auto ii = readoutRequestReceived_[ring].begin();
                    while (active && ii != readoutRequestReceived_[ring].end())
                    {
                        bool found = false;
                        uint64_t ts = *ii;
                        for (auto roc : DTCLib::DTC_ROCS)
                        {
                            drMutex_.lock();
                            if (dataRequestReceived_[ring][roc].count(ts))
                            {
                                drMutex_.unlock();
                                activeTimestamps.insert(ts);
                                found = true;
                                break;
                            }
                            drMutex_.unlock();
                        }
                        if (!found) active = false;
                        ++ii;
                    }
                }
            }
            rrMutex_.unlock();

            bool exitLoop = false;
            for (auto ts : activeTimestamps)
            {
                if (exitLoop) break;
                for (int ring = 0; ring <= DTCLib::DTC_Ring_5; ++ring)
                {
                    if (exitLoop) break;
                    rrMutex_.lock();
                    if (readoutRequestReceived_[ring].count(ts) > 0 && currentOffset < sizeof(mu2e_databuff_t))
                    {
                        for (int roc = 0; roc <= DTCLib::DTC_ROC_5; ++roc)
                        {
                            drMutex_.lock();
                            if (dataRequestReceived_[ring][roc].count(ts) > 0 && currentOffset < sizeof(mu2e_databuff_t))
                            {
                                TRACE(17, "mu2esim::read_data, DAQ Channel w/Requests recieved");
                                uint8_t packet[16];

                                int nSamples = rand() % 10 + 10;
                                uint16_t nPackets = 1;
                                if (mode_ == DTCLib::DTC_SimMode_Calorimeter) {
                                    if (nSamples <= 5) { nPackets = 1; }
                                    else { nPackets = floor((nSamples - 6) / 8 + 2); }
                                }
                                else if (mode_ == DTCLib::DTC_SimMode_Performance) {
                                    nPackets = dataRequestReceived_[ring][roc][ts];
                                }
                                if ((currentOffset + (nPackets + 1) * 16) > sizeof(mu2e_databuff_t))
                                {
                                    exitLoop = true;
                                    drMutex_.unlock();
                                    break;
                                }

                                // Record the DataBlock size
                                uint16_t dataBlockByteCount = static_cast<uint16_t>((nPackets + 1) * 16);
                                TRACE(17, "mu2esim::read_data DataBlock size is %u", dataBlockByteCount);

                                // Add a Data Header packet to the reply
                                packet[0] = static_cast<uint8_t>(dataBlockByteCount & 0xFF);
                                packet[1] = static_cast<uint8_t>(dataBlockByteCount >> 8);
                                packet[2] = 0x50 + (roc & 0x0F);
                                packet[3] = 0x80 + (ring & 0x0F);
                                packet[4] = static_cast<uint8_t>(nPackets & 0xFF);
                                packet[5] = static_cast<uint8_t>(nPackets >> 8);
                                DTCLib::DTC_Timestamp dts(ts);
                                dts.GetTimestamp(packet, 6);
                                packet[12] = 0;
                                packet[13] = 0;
                                packet[14] = 0;
                                packet[15] = 0;

                                TRACE(17, "mu2esim::read_data Copying Data Header packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                    , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet[0], sizeof(packet));
                                currentOffset += sizeof(packet);
                                buffSize_[chn][hwIdx_[chn]] = currentOffset;

                                switch (mode_)
                                {
                                case DTCLib::DTC_SimMode_CosmicVeto:
                                {
                                    nSamples = 4;
                                    packet[0] = static_cast<uint8_t>(simIndex_[ring][roc]);
                                    packet[1] = static_cast<uint8_t>(simIndex_[ring][roc] >> 8);
                                    packet[2] = 0x0; // No TDC value!
                                    packet[3] = 0x0;
                                    packet[4] = static_cast<uint8_t>(nSamples);
                                    packet[5] = static_cast<uint8_t>(nSamples >> 8);

                                    packet[6] = 0;
                                    packet[7] = 0;
                                    packet[8] = static_cast<uint8_t>(simIndex_[ring][roc]);
                                    packet[9] = static_cast<uint8_t>(simIndex_[ring][roc] >> 8);
                                    packet[10] = 2;
                                    packet[11] = 2;
                                    packet[12] = static_cast<uint8_t>(3 * simIndex_[ring][roc]);
                                    packet[13] = static_cast<uint8_t>((3 * simIndex_[ring][roc]) >> 8);
                                    packet[14] = 0;
                                    packet[15] = 0;

                                    TRACE(17, "mu2esim::read_data Copying Data packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                        , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                    memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet, sizeof(packet));
                                    currentOffset += sizeof(packet);
                                    buffSize_[chn][hwIdx_[chn]] = currentOffset;
                                }
                                break;
                                case DTCLib::DTC_SimMode_Calorimeter:
                                {
                                    packet[0] = static_cast<uint8_t>(simIndex_[ring][roc]);
                                    packet[1] = ((simIndex_[ring][roc] >> 8) & 0xF) + ((simIndex_[ring][roc] & 0xF) << 4);
                                    packet[2] = 0x0; // No TDC value!
                                    packet[3] = 0x0;
                                    packet[4] = static_cast<uint8_t>(nSamples);
                                    packet[5] = static_cast<uint8_t>(nSamples >> 8);

                                    packet[6] = 0;
                                    packet[7] = 0;
                                    packet[8] = static_cast<uint8_t>(simIndex_[ring][roc]);
                                    packet[9] = static_cast<uint8_t>(simIndex_[ring][roc] >> 8);
                                    packet[10] = 2;
                                    packet[11] = 2;
                                    packet[12] = static_cast<uint8_t>(3 * simIndex_[ring][roc]);
                                    packet[13] = static_cast<uint8_t>((3 * simIndex_[ring][roc]) >> 8);
                                    packet[14] = 4;
                                    packet[15] = 4;

                                    TRACE(17, "mu2esim::read_data Copying Data packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                        , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                    memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet, sizeof(packet));
                                    currentOffset += sizeof(packet);
                                    buffSize_[chn][hwIdx_[chn]] = currentOffset;

                                    int samplesProcessed = 5;
                                    for (int i = 1; i < nPackets; ++i)
                                    {
                                        packet[0] = static_cast<uint8_t>(samplesProcessed * simIndex_[ring][roc]);
                                        packet[1] = static_cast<uint8_t>((samplesProcessed * simIndex_[ring][roc]) >> 8);
                                        packet[2] = static_cast<uint8_t>(samplesProcessed + 1);
                                        packet[3] = static_cast<uint8_t>(samplesProcessed + 1);
                                        packet[4] = static_cast<uint8_t>((2 + samplesProcessed) * simIndex_[ring][roc]);
                                        packet[5] = static_cast<uint8_t>(((2 + samplesProcessed) * simIndex_[ring][roc]) >> 8);
                                        packet[6] = static_cast<uint8_t>(samplesProcessed + 3);
                                        packet[7] = static_cast<uint8_t>(samplesProcessed + 3);
                                        packet[8] = static_cast<uint8_t>((4 + samplesProcessed) * simIndex_[ring][roc]);
                                        packet[9] = static_cast<uint8_t>(((4 + samplesProcessed) * simIndex_[ring][roc]) >> 8);
                                        packet[10] = static_cast<uint8_t>(samplesProcessed + 5);
                                        packet[11] = static_cast<uint8_t>(samplesProcessed + 5);
                                        packet[12] = static_cast<uint8_t>((6 + samplesProcessed) * simIndex_[ring][roc]);
                                        packet[13] = static_cast<uint8_t>(((6 + samplesProcessed) * simIndex_[ring][roc]) >> 8);
                                        packet[14] = static_cast<uint8_t>(samplesProcessed + 7);
                                        packet[15] = static_cast<uint8_t>(samplesProcessed + 7);


                                        samplesProcessed += 8;
                                        TRACE(17, "mu2esim::read_data Copying Data packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                            , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                        memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet, sizeof(packet));
                                        currentOffset += sizeof(packet);
                                        buffSize_[chn][hwIdx_[chn]] = currentOffset;
                                    }

                                }
                                break;
                                case DTCLib::DTC_SimMode_Tracker:
                                {
                                    packet[0] = static_cast<uint8_t>(simIndex_[ring][roc]);
                                    packet[1] = static_cast<uint8_t>(simIndex_[ring][roc] >> 8);

                                    packet[2] = 0x0; // No TDC value!
                                    packet[3] = 0x0;
                                    packet[4] = 0x0;
                                    packet[5] = 0x0;

                                    uint16_t pattern0 = 0;
                                    uint16_t pattern1 = simIndex_[ring][roc];
                                    uint16_t pattern2 = 2;
                                    uint16_t pattern3 = (simIndex_[ring][roc] * 3) % 0x3FF;
                                    uint16_t pattern4 = 4;
                                    uint16_t pattern5 = (simIndex_[ring][roc] * 5) % 0x3FF;
                                    uint16_t pattern6 = 6;
                                    uint16_t pattern7 = (simIndex_[ring][roc] * 7) % 0x3FF;

                                    packet[6] = static_cast<uint8_t>(pattern0);
                                    packet[7] = static_cast<uint8_t>((pattern0 >> 8) + (pattern1 << 2));
                                    packet[8] = static_cast<uint8_t>((pattern1 >> 6) + (pattern2 << 4));
                                    packet[9] = static_cast<uint8_t>((pattern2 >> 4) + (pattern3 << 6));
                                    packet[10] = static_cast<uint8_t>((pattern3 >> 2));
                                    packet[11] = static_cast<uint8_t>(pattern4);
                                    packet[12] = static_cast<uint8_t>((pattern4 >> 8) + (pattern5 << 2));
                                    packet[13] = static_cast<uint8_t>((pattern5 >> 6) + (pattern6 << 4));
                                    packet[14] = static_cast<uint8_t>((pattern6 >> 4) + (pattern7 << 6));
                                    packet[15] = static_cast<uint8_t>((pattern7 >> 2));

                                    TRACE(17, "mu2esim::read_data Copying Data packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                        , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                    memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet, sizeof(packet));
                                    currentOffset += sizeof(packet);
                                    buffSize_[chn][hwIdx_[chn]] = currentOffset;
                                }
                                break;
                                case DTCLib::DTC_SimMode_Performance:
                                    for (uint16_t ii = 0; ii < nPackets; ++ii)
                                    {
                                        packet[0] = static_cast<uint8_t>(ii);
                                        packet[1] = 0x11;
                                        packet[2] = 0x22;
                                        packet[3] = 0x33;
                                        packet[4] = 0x44;
                                        packet[5] = 0x55;
                                        packet[6] = 0x66;
                                        packet[7] = 0x77;
                                        packet[8] = 0x88;
                                        packet[9] = 0x99;
                                        packet[10] = 0xaa;
                                        packet[11] = 0xbb;
                                        packet[12] = 0xcc;
                                        packet[13] = 0xdd;
                                        packet[14] = 0xee;
                                        packet[15] = 0xff;

                                        TRACE(17, "mu2esim::read_data Copying Data packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                                            , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)packet, currentOffset);
                                        memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &packet, sizeof(packet));
                                        currentOffset += sizeof(packet);
                                        buffSize_[chn][hwIdx_[chn]] = currentOffset;
                                    }
                                    break;
                                case DTCLib::DTC_SimMode_Disabled:
                                default:
                                    break;
                                }
                                simIndex_[ring][roc] = (simIndex_[ring][roc] + 1) % 0x3FF;
                                TRACE(17, "mu2esim::read_data: Erasing DTC_Timestamp %li from DataRequestReceived list", ts);
                                dataRequestReceived_[ring][roc].erase(ts);

                            }
                            drMutex_.unlock();
                        }
                        if (exitLoop)
                        {
                            rrMutex_.unlock();
                            break;
                        }
                        TRACE(17, "mu2esim::read_data: Erasing DTC_Timestamp %li from ReadoutRequestReceived list", ts);
                        readoutRequestReceived_[ring].erase(ts);
                    }
                    rrMutex_.unlock();
                }
            }
        }
        else if (chn == 1)
        {
            bool exitLoop = false;
            for (int ring = 0; ring <= DTCLib::DTC_Ring_5; ++ring)
            {
                if (exitLoop) break;
                for (int roc = 0; roc <= DTCLib::DTC_ROC_5; ++roc)
                {
                    if (dcsRequestReceived_[ring][roc])
                    {
                        if (currentOffset + 16 >= sizeof(mu2e_databuff_t)) {
                            exitLoop = true;
                            break;
                        }
                        TRACE(17, "mu2esim::read_data DCS Request Recieved, Sending Response");
                        uint8_t replyPacket[16];
                        replyPacket[0] = 16;
                        replyPacket[1] = 0;
                        replyPacket[2] = 0x40;
                        replyPacket[3] = (ring & 0x0F) + 0x80;
                        for (int i = 4; i < 16; ++i)
                        {
                            replyPacket[i] = dcsRequest_[ring][roc].GetData()[i - 4];
                        }

                        TRACE(17, "mu2esim::read_data Copying DCS Reply packet into buffer, chn=%i, idx=%li, buf=%p, packet=%p, off=%li"
                            , chn, hwIdx_[chn], (void*)dmaData_[chn][hwIdx_[chn]], (void*)replyPacket, currentOffset);
                        memcpy((char*)dmaData_[chn][hwIdx_[chn]] + currentOffset, &replyPacket, sizeof(replyPacket));
                        currentOffset += sizeof(replyPacket);
                        buffSize_[chn][hwIdx_[chn]] = currentOffset;
                        dcsRequestReceived_[ring][roc] = false;

                    }
                }
            }
        }
    }

    TRACE(17, "mu2esim::read_data Setting output buffer to dmaData_[%i][%li]=%p, retsts=%lu", chn, swIdx_[chn], (void*)dmaData_[chn][swIdx_[chn]], buffSize_[chn][swIdx_[chn]]);
    uint64_t bytesReturned = buffSize_[chn][swIdx_[chn]];
    memcpy(dmaData_[chn][swIdx_[chn]], (uint64_t*)&bytesReturned, sizeof(uint64_t));
    *buffer = dmaData_[chn][swIdx_[chn]];
    swIdx_[chn] = (swIdx_[chn] + 1) % SIM_BUFFCOUNT;
#ifdef _WIN32
    return 1;
#else
    return bytesReturned;
#endif
}

int mu2esim::write_data(int chn, void *buffer, size_t bytes)
{
    TRACE(17, "mu2esim::write_data start");
    uint32_t worda;
    memcpy(&worda, buffer, sizeof(worda));
    uint16_t word = static_cast<uint16_t>(worda >> 16);
    TRACE(17, "mu2esim::write_data worda is 0x%x and word is 0x%x", worda, word);

    switch (chn) {
    case 0: // DAQ Channel
    {
        DTCLib::DTC_Timestamp ts((uint8_t*)buffer + 6);
        if ((word & 0x8010) == 0x8010) {
            int activeDAQRing = (word & 0x0F00) >> 8;
            TRACE(17, "mu2esim::write_data: Readout Request: activeDAQRing=%i, ts=%li", activeDAQRing, ts.GetTimestamp(true));
            rrMutex_.lock();
            readoutRequestReceived_[activeDAQRing].insert(ts.GetTimestamp(true));
            rrMutex_.unlock();
        }
        else if ((word & 0x8020) == 0x8020) {
            DTCLib::DTC_Ring_ID activeDAQRing = static_cast<DTCLib::DTC_Ring_ID>((word & 0x0F00) >> 8);
            TRACE(17, "mu2esim::write_data: Data Request: activeDAQRing=%i, ts=%li", activeDAQRing, ts.GetTimestamp(true));
            if (activeDAQRing != DTCLib::DTC_Ring_Unused)
            {
                rrMutex_.lock();
                if (readoutRequestReceived_[activeDAQRing].count(ts.GetTimestamp(true)) == 0)
                {
                    TRACE(17, "mu2esim::write_data: Data Request Received but missing Readout Request!");
                }
                rrMutex_.unlock();
                DTCLib::DTC_ROC_ID activeROC = static_cast<DTCLib::DTC_ROC_ID>(word & 0xF);
                drMutex_.lock();
                if (activeROC != DTCLib::DTC_ROC_Unused)
                {
                    uint16_t packetCount = *((uint16_t*)buffer + 7);
                    dataRequestReceived_[activeDAQRing][activeROC][ts.GetTimestamp(true)] = packetCount;
                }
                drMutex_.unlock();
            }
        }
        break;
    }
    case 1:
    {
        if ((word & 0x0080) == 0) {
            DTCLib::DTC_Ring_ID activeDCSRing = static_cast<DTCLib::DTC_Ring_ID>((word & 0x0F00) >> 8);
            DTCLib::DTC_ROC_ID activeDCSROC = static_cast<DTCLib::DTC_ROC_ID>(word & 0xF);
            TRACE(17, "mu2esim::write_data activeDCSRing is %i, roc is %i", activeDCSRing, activeDCSROC);
            if (activeDCSRing != DTCLib::DTC_Ring_Unused && activeDCSROC != DTCLib::DTC_ROC_Unused) {
                dcsRequestReceived_[activeDCSRing][activeDCSROC] = true;
                uint8_t data[12];
                memcpy(&data[0], (char*)buffer + (2 * sizeof(uint16_t)), sizeof(data));
                dcsRequest_[activeDCSRing][activeDCSROC] = DTCLib::DTC_DCSRequestPacket((DTCLib::DTC_Ring_ID)activeDCSRing, (DTCLib::DTC_ROC_ID)activeDCSROC, data);
                TRACE(17, "mu2esim::write_data: Recieved DCS Request:");
                TRACE(17, dcsRequest_[activeDCSRing][activeDCSROC].toJSON().c_str());
            }
        }
        break;
    }
    }

    return 0;
}

int mu2esim::read_release(int chn, unsigned num)
{
    //Always succeeds
    TRACE(17, "mu2esim::read_release: Simulating a release of %u buffers of channel %i", num, chn);
    for (unsigned ii = 0; ii < num; ++ii) {
        swIdx_[chn] = (swIdx_[chn] + 1) % SIM_BUFFCOUNT;
    }
    return 0;
}

int mu2esim::release_all(int chn)
{
    read_release(chn, SIM_BUFFCOUNT);
    hwIdx_[chn] = 0;
    swIdx_[chn] = 0;
    return 0;
}

int  mu2esim::read_register(uint16_t address, int tmo_ms, uint32_t *output)
{
    *output = 0;
    if (registers_.count(address) > 0)
    {
        TRACE(17, "mu2esim::read_register: Returning value 0x%x for address 0x%x", registers_[address], address);
        *output = registers_[address];
        return 0;
    }
    return 1;
}

int  mu2esim::write_register(uint16_t address, int tmo_ms, uint32_t data)
{
    // Write the register!!!
    TRACE(17, "mu2esim::write_register: Writing value 0x%x into address 0x%x", data, address);
    registers_[address] = data;
    return 0;
}

int mu2esim::read_pcie_state(m_ioc_pcistate_t *output)
{
    *output = pcieState_;
    return 0;
}

int mu2esim::read_dma_state(int chn, int dir, m_ioc_engstate_t *output)
{
    *output = dmaState_[chn][dir];
    return 0;
}

int mu2esim::read_dma_stats(m_ioc_engstats_t *output)
{
    int engi[4] {0, 0, 0, 0};

    for (int i = 0; i < output->Count; ++i)
    {
        DMAStatistics thisStat;
        int eng = rand() % 4;
        ++(engi[eng]);
        switch (eng) {
        case 0:
            thisStat.Engine = 0;
            break;
        case 1:
            thisStat.Engine = 1;
            break;
        case 2:
            thisStat.Engine = 32;
            break;
        case 3:
            thisStat.Engine = 33;
            break;
        }
        thisStat.LWT = 0;
        thisStat.LBR = engi[eng] * 1000000;
        thisStat.LAT = thisStat.LBR / 1000000;
        output->engptr[i] = thisStat;
    }

    return 0;
}

int mu2esim::read_trn_stats(TRNStatsArray *output)
{

    for (int i = 0; i < output->Count; ++i){
        TRNStatistics thisStat;
        thisStat.LRX = (i + 1) * 1000000;
        thisStat.LTX = (i + 1) * 1000000;
        output->trnptr[i] = thisStat;
    }

    return 0;
}

int mu2esim::read_test_command(m_ioc_cmd_t *output)
{
    *output = testState_;
    return 0;
}

int mu2esim::write_test_command(m_ioc_cmd_t input, bool start)
{
    testState_ = input;
    testStarted_ = start;
    return 0;
}

unsigned mu2esim::delta_(int chn, int dir)
{
    unsigned hw = hwIdx_[chn];
    unsigned sw = swIdx_[chn];
    TRACE(21, "mu2esim::delta_ chn=%d dir=%d hw=%u sw=%u num_buffs=%u"
        , chn, dir, hw, sw, SIM_BUFFCOUNT);
    if (dir == C2S)
        return ((hw >= sw)
        ? hw - sw
        : SIM_BUFFCOUNT + hw - sw);
    else
        return ((sw >= hw)
        ? SIM_BUFFCOUNT - (sw - hw)
        : hw - sw);
}

void mu2esim::clearBuffer_(int chn, bool increment) {
    // Clear the buffer:
    TRACE(17, "mu2esim::clearBuffer_: Clearing output buffer");
    if (increment) {
        hwIdx_[chn] = (hwIdx_[chn] + 1) % SIM_BUFFCOUNT;
    }
    memset(dmaData_[chn][hwIdx_[chn]], 0, sizeof(mu2e_databuff_t));
}
