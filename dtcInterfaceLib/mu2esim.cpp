// This file (mu2edev.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb 13, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

/*
 *    make mu2edev.o CFLAGS='-g -Wall -std=c++0x'
 */

#include "TRACE/tracemf.h"
#define TRACE_NAME "mu2esim"

#define TLVL_Constructor TLVL_DEBUG + 2
#define TLVL_Init TLVL_DEBUG + 3
#define TLVL_ReadData TLVL_DEBUG + 4
#define TLVL_ReadData2 TLVL_DEBUG + 5
#define TLVL_WriteData TLVL_DEBUG + 6
#define TLVL_ReadRelease TLVL_DEBUG + 7
#define TLVL_ReadRegister TLVL_DEBUG + 8
#define TLVL_ReadRegister2 TLVL_DEBUG + 9
#define TLVL_WriteRegister TLVL_DEBUG + 10
#define TLVL_WriteRegister2 TLVL_DEBUG + 11
#define TLVL_CFOEmulator TLVL_DEBUG + 12
#define TLVL_CFOEmulator2 TLVL_DEBUG + 13
#define TLVL_DeltaChn TLVL_DEBUG + 14
#define TLVL_ClearBuffer TLVL_DEBUG + 15
#define TLVL_ClearBuffer2 TLVL_DEBUG + 16
#define TLVL_OpenEvent TLVL_DEBUG + 17
#define TLVL_CloseEvent TLVL_DEBUG + 18
#define TLVL_DCSPacketSimulator TLVL_DEBUG + 19
#define TLVL_PacketSimulator TLVL_DEBUG + 20


#include "mu2esim.h"
#include "DTC_Registers.h"

#include <cmath>
#include <vector>

#define THREADED_CFO_EMULATOR 0

mu2esim::mu2esim(std::string ddrFileName)
	: registers_()
	, swIdx_()
	/*, detSimLoopCount_(0)*/
	, dmaData_()
	, ddrFileName_(ddrFileName)
	, ddrFile_(nullptr)
	, mode_(DTCLib::DTC_SimMode_Disabled)
	, simIndex_()
	, cancelCFO_(true)
	, readoutRequestReceived_()
	, currentTimestamp_(0xFFFFFFFFFFFF)
	, currentEventSize_(0)
	, eventBegin_(0)
{
	TLOG(TLVL_Constructor) << "mu2esim::mu2esim BEGIN";
	swIdx_[0] = 0;
	swIdx_[1] = 0;
	for (unsigned ii = 0; ii < SIM_BUFFCOUNT; ++ii)
	{
		dmaData_[0][ii] = reinterpret_cast<mu2e_databuff_t*>(new char[0x10000]);
		dmaData_[1][ii] = reinterpret_cast<mu2e_databuff_t*>(new char[0x10000]);
	}
	release_all(0);
	release_all(1);
	for (auto link = 0; link < 6; ++link)
	{
		simIndex_[link] = 0;
	}

	TLOG(TLVL_INFO) << "Going to open simulated RAM file " << ddrFileName_;
	ddrFile_ = std::make_unique<std::fstream>(ddrFileName_, std::ios::binary | std::ios::in | std::ios::out);

	if (!ddrFile_->is_open() || ddrFile_->fail())
	{
		TLOG(TLVL_INFO) << "File " << ddrFileName_ << " does not exist, creating";
		ddrFile_->open(ddrFileName_, std::fstream::binary | std::fstream::trunc | std::fstream::out);
		ddrFile_->close();
		// re-open with original flags
		ddrFile_->open(ddrFileName_, std::fstream::binary | std::fstream::in | std::fstream::out);
	}
	eventBegin_ = ddrFile_->tellp();

	TLOG(TLVL_Constructor) << "mu2esim::mu2esim END";
}

mu2esim::~mu2esim()
{
	cancelCFO_ = true;
	if (cfoEmulatorThread_.joinable()) cfoEmulatorThread_.join();
	for (unsigned ii = 0; ii < SIM_BUFFCOUNT; ++ii)
	{
		delete[] dmaData_[0][ii];
		delete[] dmaData_[1][ii];
	}
	ddrFile_->close();
}

int mu2esim::init(DTCLib::DTC_SimMode mode)
{
	TLOG(TLVL_Init) << "mu2e Simulator::init";
	mode_ = mode;

	TLOG(TLVL_Init) << "Initializing registers";
	// Set initial register values...
	registers_[DTCLib::DTC_Register_DesignVersion] = 0x00006363;  // v99.99
	registers_[DTCLib::DTC_Register_DesignDate] = 0x53494D44;     // SIMD in ASCII
	registers_[DTCLib::DTC_Register_DesignStatus] = 0,
	registers_[DTCLib::DTC_Register_VivadoVersion] = 0,
	registers_[DTCLib::DTC_Register_FPGA_Temperature] = 0,
	registers_[DTCLib::DTC_Register_FPGA_VCCINT] = 0,
	registers_[DTCLib::DTC_Register_FPGA_VCCAUX] = 0,
	registers_[DTCLib::DTC_Register_FPGA_VCCBRAM] = 0,
	registers_[DTCLib::DTC_Register_FPGA_MonitorAlarm] = 0,
	registers_[DTCLib::DTC_Register_DTCControl] = 0x00000003;                  // System Clock, Timing Enable
	registers_[DTCLib::DTC_Register_DMATransferLength] = 0x80000010;           // Default value from HWUG
	registers_[DTCLib::DTC_Register_SERDESLoopbackEnable] = 0x00000000;        // SERDES Loopback Disabled
	registers_[DTCLib::DTC_Register_ClockOscillatorStatus] = 0x20002;          // Initialization Complete, no IIC Error
	registers_[DTCLib::DTC_Register_ROCEmulationEnable] = 0x3F;                // ROC Emulators enabled (of course!)
	registers_[DTCLib::DTC_Register_LinkEnable] = 0x3F3F;                      // All links Tx/Rx enabled, CFO and timing disabled
	registers_[DTCLib::DTC_Register_SERDES_Reset] = 0x0;                       // No SERDES Reset
	registers_[DTCLib::DTC_Register_SERDES_RXDisparityError] = 0x0;            // No SERDES Disparity Error
	registers_[DTCLib::DTC_Register_SERDES_RXCharacterNotInTableError] = 0x0;  // No SERDES CNIT Error
	registers_[DTCLib::DTC_Register_SERDES_UnlockError] = 0x0;                 // No SERDES Unlock Error
	registers_[DTCLib::DTC_Register_SERDES_PLLLocked] = 0x7F;                  // SERDES PLL Locked
	registers_[DTCLib::DTC_Register_SERDES_PLLPowerDown] = 0;
	registers_[DTCLib::DTC_Register_SERDES_RXStatus] = 0x0;                // SERDES RX Status Nominal
	registers_[DTCLib::DTC_Register_SERDES_ResetDone] = 0xFFFFFFFF;        // SERDES Resets Done
	registers_[DTCLib::DTC_Register_SERDES_RXCDRLockStatus] = 0x7F00007F;  // RX CDR Locked
	registers_[DTCLib::DTC_Register_DMATimeoutPreset] = 0x800;             // DMA Timeout Preset
	registers_[DTCLib::DTC_Register_ROCReplyTimeout] = 0x200000;           // ROC Timeout Preset
	registers_[DTCLib::DTC_Register_ROCReplyTimeoutError] = 0x0;           // ROC Timeout Error
	registers_[DTCLib::DTC_Register_LinkPacketLength] = 0x10;
	registers_[DTCLib::DTC_Register_EVBPartitionID] = 0x0;
	registers_[DTCLib::DTC_Register_EVBDestCount] = 0x0;
	registers_[DTCLib::DTC_Register_SERDESOscillatorIICBusControl] = 0;
	registers_[DTCLib::DTC_Register_SERDESOscillatorIICBusLow] = 0xFFFFFFFF;
	registers_[DTCLib::DTC_Register_SERDESOscillatorIICBusHigh] = 0x77f3f;
	registers_[DTCLib::DTC_Register_DDROscillatorReferenceFrequency] = 0xbebc200;
	registers_[DTCLib::DTC_Register_DDROscillatorIICBusControl] = 0;
	registers_[DTCLib::DTC_Register_DDROscillatorIICBusLow] = 0x1074f43b;
	registers_[DTCLib::DTC_Register_DDROscillatorIICBusHigh] = 0x30303;
	registers_[DTCLib::DTC_Register_TimestampPreset0] = 0x0;  // Timestamp preset to 0
	registers_[DTCLib::DTC_Register_TimestampPreset1] = 0x0;
	registers_[DTCLib::DTC_Register_DataPendingTimer] = 0x00002000;  // Data pending timeout preset
	registers_[DTCLib::DTC_Register_NUMROCs] = 0x1;                  // NUMROCs 0 for all links,except Link 0 which has 1
	registers_[DTCLib::DTC_Register_FIFOFullErrorFlag0] = 0x0;       // NO FIFO Full flags
	registers_[DTCLib::DTC_Register_FIFOFullErrorFlag1] = 0x0;
	registers_[DTCLib::DTC_Register_FIFOFullErrorFlag2] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketError] = 0x0;        // Receive Packet Error
	registers_[DTCLib::DTC_Register_CFOEmulationTimestampLow] = 0x0;  // CFO Emulation Registers
	registers_[DTCLib::DTC_Register_CFOEmulationTimestampHigh] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationRequestInterval] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationNumRequests] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks10] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks32] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks54] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationEventMode1] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationEventMode2] = 0x0;
	registers_[DTCLib::DTC_Register_CFOEmulationDebugPacketType] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationDMACount] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationDelayCount] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationControl0] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationControl1] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink0] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink1] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink2] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink3] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink4] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataLink5] = 0x0;
	registers_[DTCLib::DTC_Register_ReceiveByteCountDataCFO] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink0] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink1] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink2] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink3] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink4] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataLink5] = 0x0;
	registers_[DTCLib::DTC_Register_ReceivePacketCountDataCFO] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink0] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink1] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink2] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink3] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink4] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataLink5] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitByteCountDataCFO] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink0] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink1] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink2] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink3] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink4] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataLink5] = 0x0;
	registers_[DTCLib::DTC_Register_TransmitPacketCountDataCFO] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationDataStartAddress] = 0x0;
	registers_[DTCLib::DTC_Register_DetEmulationDataEndAddress] = 0x0;
	registers_[DTCLib::DTC_Register_EthernetFramePayloadSize] = 0x5D4;
	registers_[DTCLib::DTC_Register_FPGAProgramData] = 0x0;
	registers_[DTCLib::DTC_Register_FPGAPROMProgramStatus] = 0x1;
	registers_[DTCLib::DTC_Register_FPGACoreAccess] = 0x0;  // FPGA Core Access OK
	registers_[DTCLib::DTC_Register_EventModeLookupTableStart] = 0;
	registers_[DTCLib::DTC_Register_EventModeLookupTableEnd] = 0;

	TLOG(TLVL_Init) << "Initialize finished";
	return 0;
}

/*****************************
   read_data
   returns number of bytes read; negative value indicates an error
   */
int mu2esim::read_data(int chn, void** buffer, int tmo_ms)
{
	auto start = std::chrono::steady_clock::now();
	size_t bytesReturned = 0;
	if (delta_(chn, C2S) == 0)
	{
		TLOG(TLVL_ReadData) << "mu2esim::read_data: Clearing output buffer";
		clearBuffer_(chn, false);

		if (chn == 0)
		{
			TLOG(TLVL_ReadData) << "mu2esim::read_data: Reading size from memory file";
			uint64_t size;
			ddrFile_->read(reinterpret_cast<char*>(&size), sizeof(uint64_t) / sizeof(char));

			TLOG(TLVL_ReadData) << "mu2esim::read_data: Size is " << size;

			if (ddrFile_->eof() || size == 0)
			{
				TLOG(TLVL_ReadData) << "mu2esim::read_data: End of file reached, looping back to start";
				ddrFile_->clear();
				ddrFile_->seekg(std::ios::beg);

				TLOG(TLVL_ReadData) << " mu2esim::read_data: Re-reading size from memory file";
				ddrFile_->read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
				TLOG(TLVL_ReadData) << "mu2esim::read_data: Size is " << size;
				if (ddrFile_->eof())
				{
					TLOG(TLVL_ReadData) << "mu2esim::read_data: 0-size file detected!";
					return -1;
				}
			}
			TLOG(TLVL_ReadData) << "Size of data is " << size << ", reading into buffer " << swIdx_[chn] << ", at "
					 << (void*)dmaData_[chn][swIdx_[chn]];
			memcpy(dmaData_[chn][swIdx_[chn]], &size, sizeof(uint64_t));
			ddrFile_->read(reinterpret_cast<char*>(dmaData_[chn][swIdx_[chn]]) + sizeof(uint64_t), size);
			bytesReturned = size + sizeof(uint64_t);
		}
		else if (chn == 1)
		{
			// Data should already be in appropriate buffer
		}
	}

	*buffer = dmaData_[chn][swIdx_[chn]];
	TLOG(TLVL_ReadData2) << "mu2esim::read_data: *buffer (" << (void*)*buffer << ") should now be equal to dmaData_[" << chn << "]["
			 << swIdx_[chn] << "] (" << (void*)dmaData_[chn][swIdx_[chn]] << ")";
	swIdx_[chn] = (swIdx_[chn] + 1) % SIM_BUFFCOUNT;

	auto duration =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	TLOG(TLVL_ReadData2) << "mu2esim::read_data took " << duration << " milliseconds out of tmo_ms=" << tmo_ms;
	return static_cast<int>(bytesReturned);
}

int mu2esim::write_data(int chn, void* buffer, size_t bytes)
{
	if (chn == 0)
	{
		TLOG(TLVL_WriteData) << "mu2esim::write_data: adding buffer to simulated DDR memory sz=" << bytes
				 << ", *buffer=" << *((uint64_t*)buffer);
		if (bytes <= sizeof(mu2e_databuff_t))
		{
			if (currentEventSize_ > 0) closeEvent_();

			// Strip off first 64-bit word
			auto writeBytes = *reinterpret_cast<uint64_t*>(buffer) - sizeof(uint64_t);
			auto ptr = reinterpret_cast<char*>(buffer) + (sizeof(uint64_t) / sizeof(char));
			ddrFile_->write(ptr, writeBytes);
			registers_[DTCLib::DTC_Register_DetEmulationDataEndAddress] += static_cast<uint32_t>(writeBytes);
			ddrFile_->flush();
			return 0;
		}
		TLOG(TLVL_WriteData) << "mu2esim::write_data: I was asked to write more than one buffer's worth of data. Cowardly refusing.";
	}
	else if (chn == 1)
	{
		TLOG(TLVL_WriteData) << "mu2esim::write_data start: chn=" << chn << ", buf=" << buffer << ", bytes=" << bytes;
		uint32_t worda;
		memcpy(&worda, buffer, sizeof worda);
		auto word = static_cast<uint16_t>(worda >> 16);
		TLOG(TLVL_WriteData) << "mu2esim::write_data worda is 0x" << std::hex << worda << " and word is 0x" << std::hex << word;
		auto activeLink = static_cast<DTCLib::DTC_Link_ID>((word & 0x0F00) >> 8);

		DTCLib::DTC_Timestamp ts(reinterpret_cast<uint8_t*>(buffer) + 6);
		if ((word & 0x8010) == 0x8010)
		{
			TLOG(TLVL_WriteData) << "mu2esim::write_data: Readout Request: activeDAQLink=" << activeLink
					 << ", ts=" << ts.GetTimestamp(true);
			readoutRequestReceived_[ts.GetTimestamp(true)][activeLink] = true;
		}
		else if ((word & 0x8020) == 0x8020)
		{
			TLOG(TLVL_WriteData) << "mu2esim::write_data: Data Request: activeDAQLink=" << activeLink << ", ts=" << ts.GetTimestamp(true);
			if (activeLink != DTCLib::DTC_Link_Unused)
			{
				if (!readoutRequestReceived_[ts.GetTimestamp(true)][activeLink])
				{
					TLOG(TLVL_WriteData) << "mu2esim::write_data: Data Request Received but missing Readout Request!";
				}
				else
				{
					openEvent_(ts);

					auto packetCount = *(reinterpret_cast<uint16_t*>(buffer) + 7);
					packetSimulator_(ts, activeLink, packetCount);

					readoutRequestReceived_[ts.GetTimestamp(true)][activeLink] = false;
					if (readoutRequestReceived_[ts.GetTimestamp(true)].count() == 0)
					{
						closeEvent_();
						readoutRequestReceived_.erase(ts.GetTimestamp(true));
					}
				}
			}
		}
		if ((word & 0x0080) == 0)
		{
			TLOG(TLVL_WriteData) << "mu2esim::write_data activeDCSLink is " << activeLink;
			if (activeLink != DTCLib::DTC_Link_Unused)
			{
				DTCLib::DTC_DataPacket packet(buffer);
				DTCLib::DTC_DCSRequestPacket thisPacket(packet);
				if (thisPacket.GetType() == DTCLib::DTC_DCSOperationType_Read ||
					thisPacket.GetType() == DTCLib::DTC_DCSOperationType_BlockRead || thisPacket.RequestsAck())
				{
					TLOG(TLVL_WriteData) << "mu2esim::write_data: Recieved DCS Request:";
					TLOG(TLVL_WriteData) << thisPacket.toJSON().c_str();
					dcsPacketSimulator_(thisPacket);
				}
			}
		}
	}

	return 0;
}

int mu2esim::read_release(int chn, unsigned num)
{
	// Always succeeds
	TLOG(TLVL_ReadRelease) << "mu2esim::read_release: Simulating a release of " << num << "u buffers of channel " << chn;
	for (unsigned ii = 0; ii < num; ++ii)
	{
		if (delta_(chn, C2S) != 0) swIdx_[chn] = (swIdx_[chn] + 1) % SIM_BUFFCOUNT;
	}
	return 0;
}

int mu2esim::release_all(int chn)
{
	read_release(chn, SIM_BUFFCOUNT);
	swIdx_[chn] = 0;
	return 0;
}

int mu2esim::read_register(uint16_t address, int tmo_ms, uint32_t* output)
{
	auto start = std::chrono::steady_clock::now();
	*output = 0;
	if (registers_.count(address) > 0)
	{
		TLOG(TLVL_ReadRegister) << "mu2esim::read_register: Returning value 0x" << std::hex << registers_[address] << " for address 0x"
				 << std::hex << address;
		*output = registers_[address];
		return 0;
	}
	auto duration =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	TLOG(TLVL_ReadRegister2) << "mu2esim::read_register took " << duration << " milliseconds out of tmo_ms=" << tmo_ms;
	return 1;
}

int mu2esim::write_register(uint16_t address, int tmo_ms, uint32_t data)
{
	auto start = std::chrono::steady_clock::now();
	// Write the register!!!
	TLOG(TLVL_WriteRegister) << "mu2esim::write_register: Writing value 0x" << std::hex << data << " into address 0x" << std::hex
			 << address;
	registers_[address] = data;
	auto duration =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	TLOG(TLVL_WriteRegister2) << "mu2esim::write_register took " << duration << " milliseconds out of tmo_ms=" << tmo_ms;
	std::bitset<32> dataBS(data);
	if (address == DTCLib::DTC_Register_DTCControl)
	{
		auto detectorEmulationMode = (registers_[DTCLib::DTC_Register_DetEmulationControl0] & 0x3) != 0;
		if (dataBS[30] == 1 && !detectorEmulationMode)
		{
			TLOG(TLVL_WriteRegister2) << "mu2esim::write_register: CFO Emulator Enable Detected!";
			cancelCFO_ = true;
			if (cfoEmulatorThread_.joinable()) cfoEmulatorThread_.join();
			cancelCFO_ = false;
#if THREADED_CFO_EMULATOR
			if (registers_[0x91AC] > 10)
			{
				cfoEmulatorThread_ = std::thread(&mu2esim::CFOEmulator_, this);
			}
			else
#endif
			{
				CFOEmulator_();
			}
		}
		else if (dataBS[26] == 1 && detectorEmulationMode)
		{
			TLOG(TLVL_WriteRegister2) << "mu2esim::write_register: IGNORING CFO Emulator Enable because we're in Detector Simulator mode!";
		}
		if (dataBS[31] == 1)
		{
			TLOG(TLVL_WriteRegister2) << "mu2esim::write_register: RESETTING DTC EMULATOR!";
			init(mode_);
			ddrFile_->close();
			ddrFile_->open(ddrFileName_, std::ios::trunc | std::ios::binary | std::ios::out | std::ios::in);
			eventBegin_ = ddrFile_->tellp();
		}
	}
	if (address == DTCLib::DTC_Register_DetEmulationControl0)
	{
		if (dataBS[0] == 0)
		{
			ddrFile_->close();
			ddrFile_->open(ddrFileName_, std::ios::trunc | std::ios::binary | std::ios::out | std::ios::in);
			eventBegin_ = ddrFile_->tellp();
		}
	}
	if (address == DTCLib::DTC_Register_DetEmulationDataStartAddress)
	{
		ddrFile_->seekg(data);
	}
	return 0;
}

void mu2esim::CFOEmulator_()
{
	if (cancelCFO_)
	{
		std::bitset<32> ctrlReg(registers_[0x9100]);
		ctrlReg[30] = 0;
		registers_[0x9100] = ctrlReg.to_ulong();
		return;
	}
	DTCLib::DTC_Timestamp start(registers_[DTCLib::DTC_Register_CFOEmulationTimestampLow],
								static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationTimestampHigh]));
	auto count = registers_[DTCLib::DTC_Register_CFOEmulationNumRequests];
	auto ticksToWait = static_cast<long long>(registers_[DTCLib::DTC_Register_CFOEmulationRequestInterval] * 0.0064);
	TLOG(TLVL_CFOEmulator) << "mu2esim::CFOEmulator_ start timestamp=" << start.GetTimestamp(true) << ", count=" << count
			 << ", delayBetween=" << ticksToWait;

	bool linkEnabled[6];
	for (auto link : DTCLib::DTC_Links)
	{
		std::bitset<32> linkRocs(registers_[DTCLib::DTC_Register_NUMROCs]);
		auto number = linkRocs[link * 3] + (linkRocs[link * 3 + 1] << 1) + (linkRocs[link * 3 + 2] << 2);
		TLOG(TLVL_CFOEmulator) << "mu2esim::CFOEmulator_ linkRocs[" << link << "]=" << number;
		linkEnabled[link] = number != 0;
	}
	unsigned sentCount = 0;
	while (sentCount < count && !cancelCFO_)
	{
		openEvent_(start + sentCount);
		for (auto link : DTCLib::DTC_Links)
		{
			uint16_t packetCount;
			switch (link)
			{
				case DTCLib::DTC_Link_0:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks10]);
					break;
				case DTCLib::DTC_Link_1:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks10] >> 16);
					break;
				case DTCLib::DTC_Link_2:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks32]);
					break;
				case DTCLib::DTC_Link_3:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks32] >> 16);
					break;
				case DTCLib::DTC_Link_4:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks54]);
					break;
				case DTCLib::DTC_Link_5:
					packetCount = static_cast<uint16_t>(registers_[DTCLib::DTC_Register_CFOEmulationNumPacketsLinks54] >> 16);
					break;
				default:
					packetCount = 0;
					break;
			}
			if (linkEnabled[link] != 0)
			{
				TLOG(TLVL_CFOEmulator) << "mu2esim::CFOEmulator_ linkRocs[" << link << "]=" << linkEnabled[link];
				TLOG(TLVL_CFOEmulator2) << "mu2esim::CFOEmulator_ activating packet simulator, link=" << link
						 << ", for timestamp=" << (start + sentCount).GetTimestamp(true);
				packetSimulator_(start + sentCount, link, packetCount);
			}
		}

		closeEvent_();

		if (ticksToWait > 100)
		{
			usleep(ticksToWait);
		}
		sentCount++;
	}
	std::bitset<32> ctrlReg(registers_[0x9100]);
	ctrlReg[30] = 0;
	registers_[0x9100] = ctrlReg.to_ulong();
}

unsigned mu2esim::delta_(int chn, int dir)
{
	if (chn == 0) return 0;
	TLOG(TLVL_DeltaChn) << "delta_ " << chn << " " << dir << " = 0";
	unsigned hw = hwIdx_[chn];
	unsigned sw = swIdx_[chn];
	TLOG(TLVL_DeltaChn) << "mu2esim::delta_ chn=" << chn << " dir=" << dir << " hw=" << hw << " sw=" << sw
			 << " num_buffs=" << SIM_BUFFCOUNT;
	if (dir == C2S)
		return ((hw >= sw) ? hw - sw : SIM_BUFFCOUNT + hw - sw);
	else
		return ((sw >= hw) ? SIM_BUFFCOUNT - (sw - hw) : hw - sw);
}

void mu2esim::clearBuffer_(int chn, bool increment)
{
	// Clear the buffer:
	/*
  TLOG(TLVL_ClearBuffer) << "mu2esim::clearBuffer_: Clearing output buffer";
  if (increment)
  {
          hwIdx_[chn] = (hwIdx_[chn] + 1) " << << " SIM_BUFFCOUNT;
  }
  memset(dmaData_[chn][hwIdx_[chn]], 0, sizeof(mu2e_databuff_t));
  */
	TLOG(TLVL_ClearBuffer2) << "mu2esim::clearBuffer_(" << chn << ", " << increment << "): NOP";
}

void mu2esim::openEvent_(DTCLib::DTC_Timestamp ts)
{
	TLOG(TLVL_OpenEvent) << "mu2esim::openEvent_ Checking timestamp " << ts.GetTimestamp(true) << " vs current timestamp "
			 << currentTimestamp_.GetTimestamp(true);
	if (ts == currentTimestamp_) return;
	if (currentEventSize_ > 0) closeEvent_();

	TLOG(TLVL_OpenEvent) << "mu2esim::openEvent_: Setting up initial buffer";
	eventBegin_ = ddrFile_->tellp();
	ddrFile_->write(reinterpret_cast<char*>(&currentEventSize_), sizeof(uint64_t) / sizeof(char));

	TLOG(TLVL_OpenEvent) << "mu2esim::openEvent_: updating current timestamp";
	currentTimestamp_ = ts;
}

void mu2esim::closeEvent_()
{
	TLOG(TLVL_CloseEvent) << "mu2esim::closeEvent_: Checking current event size " << currentEventSize_;
	if (currentEventSize_ > 0 && ddrFile_)
	{
		auto currentPos = ddrFile_->tellp();
		ddrFile_->seekp(eventBegin_);
		TLOG(TLVL_CloseEvent) << "mu2esim::closeEvent_: Writing event size word";
		ddrFile_->write(reinterpret_cast<char*>(&currentEventSize_), sizeof(uint64_t) / sizeof(char));
		ddrFile_->seekp(currentPos);
		currentEventSize_ = 0;
		ddrFile_->flush();
	}
	TLOG(TLVL_CloseEvent) << "mu2esim::closeEvent_ FINISH";
}

void mu2esim::dcsPacketSimulator_(DTCLib::DTC_DCSRequestPacket in)
{
	auto packetCount = 0;
	if (in.GetType() == DTCLib::DTC_DCSOperationType_BlockRead)
	{
		packetCount = 1;
	}
	TLOG(TLVL_DCSPacketSimulator) << "mu2esim::dcsPacketSimulator_: Constructing DCS Response";
	DTCLib::DTC_DMAPacket packet(DTCLib::DTC_PacketType_DCSReply, in.GetRingID(), (1 + packetCount) * 16, true);

	TLOG(TLVL_DCSPacketSimulator) << "mu2esim::dcsPacketSimulator_: copying response into new buffer";
	auto dataPacket = packet.ConvertToDataPacket();

	dataPacket.SetWord(4, static_cast<int>(in.GetType()) + (in.RequestsAck() ? 0x8 : 0) + (in.IsDoubleOp() ? 0x4 : 0) +
							  ((packetCount & 0x2) << 6));
	dataPacket.SetWord(5, (packetCount & 0x3FC) >> 2);

	auto request1 = in.GetRequest(false);
	dataPacket.SetWord(6, request1.first & 0xFF);
	dataPacket.SetWord(7, (request1.first & 0xFF00) >> 8);
	dataPacket.SetWord(8, request1.second & 0xFF);
	dataPacket.SetWord(9, (request1.second & 0xFF00) >> 8);

	if (in.GetType() != DTCLib::DTC_DCSOperationType_BlockRead)
	{
		auto request2 = in.GetRequest(true);
		dataPacket.SetWord(10, request2.first & 0xFF);
		dataPacket.SetWord(11, (request2.first & 0xFF00) >> 8);
		dataPacket.SetWord(12, request2.second & 0xFF);
		dataPacket.SetWord(13, (request2.second & 0xFF00) >> 8);
	}
	else
	{
		for (int ii = 10; ii < dataPacket.GetSize(); ++ii)
		{
			dataPacket.SetWord(ii, ii);
		}
	}

	size_t packetSize = dataPacket.GetSize();
	*reinterpret_cast<uint64_t*>(dmaData_[1][hwIdx_[1]]) = packetSize;
	memcpy(reinterpret_cast<uint64_t*>(dmaData_[1][hwIdx_[1]]) + 1, dataPacket.GetData(), packetSize);
	hwIdx_[1] = (hwIdx_[1] + 1) % SIM_BUFFCOUNT;
}

void mu2esim::packetSimulator_(DTCLib::DTC_Timestamp ts, DTCLib::DTC_Link_ID link, uint16_t packetCount)
{
	TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_: Generating data for timestamp " << ts.GetTimestamp(true);

	uint8_t packet[16];
	auto packetSize = sizeof(packet) / sizeof(char);

	auto nSamples = rand() % 10 + 10;
	uint16_t nPackets = 1;
	if (mode_ == DTCLib::DTC_SimMode_Calorimeter)
	{
		if (nSamples <= 5)
		{
			nPackets = 1;
		}
		else
		{
			nPackets = static_cast<uint16_t>(floor((nSamples - 6) / 8 + 2));
		}
	}
	else if (mode_ == DTCLib::DTC_SimMode_Performance)
	{
		nPackets = packetCount;
	}
	else if (mode_ == DTCLib::DTC_SimMode_Timeout)
	{
		nPackets = 0;
	}

	// Record the DataBlock size
	uint16_t dataBlockByteCount = static_cast<uint16_t>((nPackets + 1) * 16);
	currentEventSize_ += dataBlockByteCount;

	if (mode_ != DTCLib::DTC_SimMode_Timeout)
	{
		// Add a Data Header packet to the reply
		packet[0] = static_cast<uint8_t>(dataBlockByteCount);
		packet[1] = static_cast<uint8_t>(dataBlockByteCount >> 8);
		packet[2] = 0x50;
		packet[3] = 0x80 + (link & 0x0F);
		packet[4] = static_cast<uint8_t>(nPackets & 0xFF);
		packet[5] = static_cast<uint8_t>(nPackets >> 8);
		ts.GetTimestamp(packet, 6);
		packet[12] = 0;
		packet[13] = 0;
		packet[14] = 0;
		packet[15] = 0;
	}
	else
	{
		packet[0] = 0xfe;
		packet[1] = 0xca;
		packet[2] = 0xfe;
		packet[3] = 0xca;
		packet[4] = 0xfe;
		packet[5] = 0xca;
		packet[6] = 0xfe;
		packet[7] = 0xca;
		packet[8] = 0xfe;
		packet[9] = 0xca;
		packet[10] = 0xfe;
		packet[11] = 0xca;
		packet[12] = 0xfe;
		packet[13] = 0xca;
		packet[14] = 0xfe;
		packet[15] = 0xca;
	}

	TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data Header packet to memory file, chn=0, packet=" << (void*)packet;
	ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);

	switch (mode_)
	{
		case DTCLib::DTC_SimMode_CosmicVeto: {
			nSamples = 4;
			packet[0] = static_cast<uint8_t>(simIndex_[link]);
			packet[1] = static_cast<uint8_t>(simIndex_[link] >> 8);
			packet[2] = 0x0;  // No TDC value!
			packet[3] = 0x0;
			packet[4] = static_cast<uint8_t>(nSamples);
			packet[5] = static_cast<uint8_t>(nSamples >> 8);
			packet[6] = 0;
			packet[7] = 0;
			packet[8] = static_cast<uint8_t>(simIndex_[link]);
			packet[9] = static_cast<uint8_t>(simIndex_[link] >> 8);
			packet[10] = 2;
			packet[11] = 2;
			packet[12] = static_cast<uint8_t>(3 * simIndex_[link]);
			packet[13] = static_cast<uint8_t>((3 * simIndex_[link]) >> 8);
			packet[14] = 0;
			packet[15] = 0;

			TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data packet to memory file, chn=0, packet=" << (void*)packet;
			ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);
		}
		break;
		case DTCLib::DTC_SimMode_Calorimeter: {
			packet[0] = static_cast<uint8_t>(simIndex_[link]);
			packet[1] = static_cast<uint8_t>((simIndex_[link] >> 8) & 0xF) + ((simIndex_[link] & 0xF) << 4);
			packet[2] = 0x0;  // No TDC value!
			packet[3] = 0x0;
			packet[4] = static_cast<uint8_t>(nSamples);
			packet[5] = static_cast<uint8_t>(nSamples >> 8);
			packet[6] = 0;
			packet[7] = 0;
			packet[8] = static_cast<uint8_t>(simIndex_[link]);
			packet[9] = static_cast<uint8_t>(simIndex_[link] >> 8);
			packet[10] = 2;
			packet[11] = 2;
			packet[12] = static_cast<uint8_t>(3 * simIndex_[link]);
			packet[13] = static_cast<uint8_t>((3 * simIndex_[link]) >> 8);
			packet[14] = 4;
			packet[15] = 4;

			TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data packet to memory file, chn=0, packet=" << (void*)packet;
			ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);

			auto samplesProcessed = 5;
			for (auto i = 1; i < nPackets; ++i)
			{
				packet[0] = static_cast<uint8_t>(samplesProcessed * simIndex_[link]);
				packet[1] = static_cast<uint8_t>((samplesProcessed * simIndex_[link]) >> 8);
				packet[2] = static_cast<uint8_t>(samplesProcessed + 1);
				packet[3] = static_cast<uint8_t>(samplesProcessed + 1);
				packet[4] = static_cast<uint8_t>((2 + samplesProcessed) * simIndex_[link]);
				packet[5] = static_cast<uint8_t>(((2 + samplesProcessed) * simIndex_[link]) >> 8);
				packet[6] = static_cast<uint8_t>(samplesProcessed + 3);
				packet[7] = static_cast<uint8_t>(samplesProcessed + 3);
				packet[8] = static_cast<uint8_t>((4 + samplesProcessed) * simIndex_[link]);
				packet[9] = static_cast<uint8_t>(((4 + samplesProcessed) * simIndex_[link]) >> 8);
				packet[10] = static_cast<uint8_t>(samplesProcessed + 5);
				packet[11] = static_cast<uint8_t>(samplesProcessed + 5);
				packet[12] = static_cast<uint8_t>((6 + samplesProcessed) * simIndex_[link]);
				packet[13] = static_cast<uint8_t>(((6 + samplesProcessed) * simIndex_[link]) >> 8);
				packet[14] = static_cast<uint8_t>(samplesProcessed + 7);
				packet[15] = static_cast<uint8_t>(samplesProcessed + 7);

				samplesProcessed += 8;
				TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data packet to memory file, chn=0, packet=" << (void*)packet;
				ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);
			}
		}
		break;
		case DTCLib::DTC_SimMode_Tracker: {
			packet[0] = static_cast<uint8_t>(simIndex_[link]);
			packet[1] = static_cast<uint8_t>(simIndex_[link] >> 8);

			packet[2] = 0x0;  // No TDC value!
			packet[3] = 0x0;
			packet[4] = 0x0;
			packet[5] = 0x0;

			uint16_t pattern0 = 0;
			auto pattern1 = simIndex_[link];
			uint16_t pattern2 = 2;
			uint16_t pattern3 = simIndex_[link] * 3 % 0x3FF;
			uint16_t pattern4 = 4;
			uint16_t pattern5 = simIndex_[link] * 5 % 0x3FF;
			uint16_t pattern6 = 6;
			uint16_t pattern7 = simIndex_[link] * 7 % 0x3FF;

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

			TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data packet to memory file, chn=0, packet=" << (void*)packet;
			ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);
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

				TLOG(TLVL_PacketSimulator) << "mu2esim::packetSimulator_ Writing Data packet to memory file, chn=0, packet=" << (void*)packet;
				ddrFile_->write(reinterpret_cast<char*>(packet), packetSize);
			}
			break;
		case DTCLib::DTC_SimMode_Timeout:
		case DTCLib::DTC_SimMode_Disabled:
		default:
			break;
	}
	ddrFile_->flush();
		simIndex_[link] = (simIndex_[link] + 1) % 0x3FF;
}
