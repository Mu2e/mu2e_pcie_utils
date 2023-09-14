#include "cfoInterfaceLib/CFO_Registers.h"

#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <cmath>
#include <iomanip>  // std::setw, std::setfill
#include <sstream>  // Convert uint to hex stLink

#include "TRACE/tracemf.h"
#define CFO_TLOG(lvl) TLOG(lvl) << "CFO " << device_.getDeviceUID() << ": "
#define TLVL_ResetCFO TLVL_DEBUG + 5
#define TLVL_AutogenDRP TLVL_DEBUG + 6
#define TLVL_SERDESReset TLVL_DEBUG + 7
#define TLVL_CalculateFreq TLVL_DEBUG + 8

CFOLib::CFO_Registers::CFO_Registers(DTC_SimMode mode, int cfo, std::string expectedDesignVersion,
									 bool skipInit, const std::string& uid)
	: device_(), simMode_(mode), dmaSize_(16)
{
	auto sim = getenv("CFOLIB_SIM_ENABLE");
	if (sim == nullptr)
	{
		// Give priority to CFOLIB_SIM_ENABLE, but go ahead and check DTCLIB_SIM_ENABLE, too
		sim = getenv("DTCLIB_SIM_ENABLE");
	}
	if (sim != nullptr)
	{
		switch (sim[0])
		{
			case '1':
			case 'e':
			case 'E':
				simMode_ = DTC_SimMode_Tracker;  // sim enabled
				break;
			case '2':
			case 'l':
			case 'L':
				simMode_ = DTC_SimMode_Loopback;
				break;
			case '0':
			default:
				simMode_ = DTC_SimMode_Disabled;
				break;
		}
	}
	TLOG(TLVL_INFO) << "CFO Sim Mode is " << DTC_SimModeConverter(simMode_).toString();

	if (cfo == -1)
	{
		auto CFOE = getenv("CFOLIB_CFO");
		if (CFOE != nullptr)
		{
			cfo = atoi(CFOE);
		}
		else
		{
			CFOE = getenv("DTCLIB_DTC");  // Check both environment variables for CFO
			if (CFOE != nullptr)
			{
				cfo = atoi(CFOE);
			}
			else
			{
				cfo = 0;
			}
		}
	}
	TLOG(TLVL_INFO) << "CFO ID is " << cfo;

	SetSimMode(expectedDesignVersion, simMode_, cfo, skipInit, (uid == ""? ("CFO"+std::to_string(cfo)):uid));
}

CFOLib::CFO_Registers::~CFO_Registers() { device_.close(); }

DTCLib::DTC_SimMode CFOLib::CFO_Registers::SetSimMode(std::string expectedDesignVersion, DTC_SimMode mode, int cfo,
													  bool skipInit, const std::string& uid)
{
	simMode_ = mode;
	device_.init(simMode_, cfo, /* simMemoryFile */ "", uid);
	if (expectedDesignVersion != "" && expectedDesignVersion != ReadDesignVersion())
	{
		throw new DTC_WrongVersionException(expectedDesignVersion, ReadDesignVersion());
	}

	if (skipInit)
	{
		CFO_TLOG(TLVL_INFO) << "SKIPPING Initializing device";
		return simMode_;
	} 

	CFO_TLOG(TLVL_DEBUG) << "Initialize requested, setting device registers acccording to sim mode " << DTC_SimModeConverter(simMode_).toString();
	for (auto link : CFO_Links)
	{
		bool LinkEnabled = ((maxDTCs_ >> (link * 4)) & 0xF) != 0;
		if (!LinkEnabled)
		{
			DisableLink(link);
		}
		else
		{
			int rocCount = (maxDTCs_ >> (link * 4)) & 0xF;
			EnableLink(link, DTC_LinkEnableMode(true, true), rocCount);
		}
		if (!LinkEnabled) SetSERDESLoopbackMode(link, DTC_SERDESLoopbackMode_Disabled);
	}

	if (simMode_ != DTC_SimMode_Disabled)
	{
		// Set up hardware simulation mode: Link 0 Tx/Rx Enabled, Loopback Enabled, ROC Emulator Enabled. All other Links
		// disabled.
		for (auto link : CFO_Links)
		{
			if (simMode_ == DTC_SimMode_Loopback)
			{
				SetSERDESLoopbackMode(link, DTC_SERDESLoopbackMode_NearPCS);
				//			SetMaxROCNumber(CFO_Link_0, CFO_ROC_0);
			}
		}
		SetInternalSystemClock();
		DisableTiming();
	}
	ReadMinDMATransferLength();

	CFO_TLOG(TLVL_DEBUG) << "Done setting device registers";
	return simMode_;
}

//
// CFO Register Dumps
//
std::string CFOLib::CFO_Registers::FormattedRegDump(int width)
{
	std::string divider(width, '=');
	formatterWidth_ = width - 27 - 65;
	if (formatterWidth_ < 28)
	{
		formatterWidth_ = 28;
	}
	std::string spaces(formatterWidth_ - 4, ' ');
	std::ostringstream o;
	o << "Memory Map: " << std::endl;
	o << "    Address | Value      | Name " << spaces << "| Translation" << std::endl;
	for (auto i : formattedDumpFunctions_)
	{
		o << divider << std::endl;
		o << i();
	}
	return o.str();
}

std::string CFOLib::CFO_Registers::LinkCountersRegDump(int width)
{
	std::string divider(width, '=');
	formatterWidth_ = width - 27 - 65;
	if (formatterWidth_ < 28)
	{
		formatterWidth_ = 28;
	}
	std::string spaces(formatterWidth_ - 4, ' ');
	std::ostringstream o;
	o << "SERDES Byte/Packet Counters: " << std::endl;
	o << "    Address | Value      | Name " << spaces << "| Translation" << std::endl;
	for (auto i : formattedCounterFunctions_)
	{
		o << divider << std::endl;
		o << i();
	}
	return o.str();
}

//
// Register IO Functions
//

// Desgin Version/Date Registers
std::string CFOLib::CFO_Registers::ReadDesignVersion()
{
	auto data = ReadRegister_(CFO_Register_DesignVersion) & 0xFFFFFF;
	int minorHex = data & 0xFF;
	auto minor = ((minorHex & 0xF0) >> 4) * 10 + (minorHex & 0xF);
	int majorHex = (data & 0xFFFF00) >> 8;
	auto major = ((majorHex & 0xF000) >> 12) * 1000 + ((majorHex & 0xF00) >> 8) * 100 + ((majorHex & 0xF0) >> 4) * 10 +
				 (majorHex & 0xF);
	std::ostringstream o;
	o << "v" << std::setw(4) << std::setfill('0') << major << "." << std::setw(2) << std::setfill('0') << minor;
	return o.str();
}

DTCLib::DTC_SerdesClockSpeed CFOLib::CFO_Registers::ReadSERDESVersion()
{
	auto data = (ReadRegister_(CFO_Register_DesignVersion) & 0xF0000000) >> 28;
	if (data == 3) return DTC_SerdesClockSpeed_3125Gbps;
	return DTC_SerdesClockSpeed_48Gbps;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDesignVersion()
{
	auto form = CreateFormatter(CFO_Register_DesignVersion);
	form.description = "CFO Firmware Design Version";
	form.vals.push_back(ReadDesignVersion());
	form.vals.push_back(std::string("Intrinsic Clock Speed: ") +
						(ReadSERDESVersion() == DTC_SerdesClockSpeed_3125Gbps ? "3.125 Gbps" : "4.8 Gbps"));
	return form;
}


/// <summary>
/// Read the modification date of the CFO firmware
/// </summary>
/// <returns>Design date in MON/DD/20YY HH:00 format</returns>
std::string CFOLib::CFO_Registers::ReadDesignDate()
{
	auto readData = ReadRegister_(CFO_Register_DesignDate);
	std::ostringstream o;
	std::vector<std::string> months({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"});
	int mon =  ((readData>>20)&0xF)*10 + ((readData>>16)&0xF);
	o << months[mon-1] << "/" << 
		((readData>>12)&0xF) << ((readData>>8)&0xF) << "/20" << 
		((readData>>28)&0xF) << ((readData>>24)&0xF) << " " <<
		((readData>>4)&0xF) << ((readData>>0)&0xF) << ":00   raw-data: 0x" << std::hex << readData;
	

	// auto data = ReadRegister_(CFO_Register_DesignDate);
	// std::ostringstream o;
	// int yearHex = (data & 0xFF000000) >> 24;
	// auto year = ((yearHex & 0xF0) >> 4) * 10 + (yearHex & 0xF);
	// int monthHex = (data & 0xFF0000) >> 16;
	// auto month = ((monthHex & 0xF0) >> 4) * 10 + (monthHex & 0xF);
	// int dayHex = (data & 0xFF00) >> 8;
	// auto day = ((dayHex & 0xF0) >> 4) * 10 + (dayHex & 0xF);
	// int hour = ((data & 0xF0) >> 4) * 10 + (data & 0xF);
	// o << "20" << std::setfill('0') << std::setw(2) << year << "-";
	// o << std::setfill('0') << std::setw(2) << month << "-";
	// o << std::setfill('0') << std::setw(2) << day << "-";
	// o << std::setfill('0') << std::setw(2) << hour;
	// // std::cout << o.str() << std::endl;
	return o.str();
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDesignDate()
{
	auto form = CreateFormatter(CFO_Register_DesignDate);
	form.description = "CFO Firmware Design Date";
	form.vals.push_back(ReadDesignDate());
	return form;
}

bool CFOLib::CFO_Registers::ReadDDRFIFOEmpty()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_DesignStatus);
	return data[2];
}

bool CFOLib::CFO_Registers::ReadDDRClockCalibrationDone()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_DesignStatus);
	return data[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDesignStatus()
{
	auto form = CreateFormatter(CFO_Register_DesignStatus);
	form.description = "Design Status Register";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("DDR FIFO Empty:             [") + (ReadDDRFIFOEmpty() ? "x" : " ") + "]");
	form.vals.push_back(std::string("DDR Clock Calibration Done: [") + (ReadDDRClockCalibrationDone() ? "x" : " ") + "]");
	return form;
}

std::string CFOLib::CFO_Registers::ReadVivadoVersion()
{
	auto data = ReadRegister_(CFO_Register_VivadoVersion);
	int yearHex = (data & 0xFFFF0000) >> 16;
	auto year = ((yearHex & 0xF000) >> 12) * 1000 + ((yearHex & 0xF00) >> 8) * 100 + ((yearHex & 0xF0) >> 4) * 10 +
				(yearHex & 0xF);
	int releaseHex = data & 0xFFFF;
	auto release = ((releaseHex & 0xF000) >> 12) * 1000 + ((releaseHex & 0xF00) >> 8) * 100 +
				   ((releaseHex & 0xF0) >> 4) * 10 + (releaseHex & 0xF);
	std::ostringstream o;
	o << year << " Release " << release;
	return o.str();
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatVivadoVersion()
{
	auto form = CreateFormatter(CFO_Register_VivadoVersion);
	form.description = "Vivado Version Register";
	form.vals.push_back(ReadVivadoVersion());
	return form;
}

// CFO Control Register
void CFOLib::CFO_Registers::ResetCFO()
{
	CFO_TLOG(TLVL_ResetCFO) << "ResetCFO start";
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[31] = 1;  // CFO Reset bit
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
	//NOTE: CFO might self-clear the reset bit
	data[31] = 0;  // CFO Reset bit
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadResetCFO()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CFOControl);
	return dataSet[31];
}

void CFOLib::CFO_Registers::ClearCFOControlRegister()
{
	WriteRegister_(0, CFO_Register_CFOControl);
}

void CFOLib::CFO_Registers::ResetCFORunPlan()
{
	CFO_TLOG(TLVL_ResetCFO) << "ResetCFO Run Plan start";
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[27] = 1;  // CFO Run Plan Reset bit
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
	data[27] = 0;  // Restore CFO Run Plan Reset bit
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadResetCFORunPlan()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CFOControl);
	return dataSet[27];
}

void CFOLib::CFO_Registers::EnableAutogenDRP()
{
	CFO_TLOG(TLVL_AutogenDRP) << "EnableAutogenDRP start";
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[23] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

void CFOLib::CFO_Registers::DisableAutogenDRP()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[23] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadAutogenDRP()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	return data[23];
}

void CFOLib::CFO_Registers::EnableEventWindowInput()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[2] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

void CFOLib::CFO_Registers::DisableEventWindowInput()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[2] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadEventWindowInput()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	return data[2];
}

void CFOLib::CFO_Registers::SetExternalSystemClock()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

void CFOLib::CFO_Registers::SetInternalSystemClock()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[1] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadSystemClock()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	return data[1];
}

void CFOLib::CFO_Registers::EnableTiming()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

void CFOLib::CFO_Registers::DisableTiming()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	data[0] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_CFOControl);
}

bool CFOLib::CFO_Registers::ReadTimingEnable()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_CFOControl);
	return data[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCFOControl()
{
	auto form = CreateFormatter(CFO_Register_CFOControl);
	form.description = "CFO Control";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("Reset:                [") + (ReadResetCFO() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Autogenerate DRP: [") + (ReadAutogenDRP() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Event Window Input:   [") + (ReadEventWindowInput() ? "x" : " ") + "]");
	form.vals.push_back(std::string("System Clock Select:  [") + (ReadSystemClock() ? "Ext" : "Int") + "]");
	form.vals.push_back(std::string("Timing Enable:        [") + (ReadTimingEnable() ? "x" : " ") + "]");
	return form;
}

// DMA Transfer Length Register
void CFOLib::CFO_Registers::SetTriggerDMATransferLength(uint16_t length)
{
	auto data = ReadRegister_(CFO_Register_DMATransferLength);
	data = (data & 0x0000FFFF) + (length << 16);
	WriteRegister_(data, CFO_Register_DMATransferLength);
}

uint16_t CFOLib::CFO_Registers::ReadTriggerDMATransferLength()
{
	auto data = ReadRegister_(CFO_Register_DMATransferLength);
	data >>= 16;
	return static_cast<uint16_t>(data);
}

void CFOLib::CFO_Registers::SetMinDMATransferLength(uint16_t length)
{
	auto data = ReadRegister_(CFO_Register_DMATransferLength);
	data = (data & 0xFFFF0000) + length;
	WriteRegister_(data, CFO_Register_DMATransferLength);
}

uint16_t CFOLib::CFO_Registers::ReadMinDMATransferLength()
{
	auto data = ReadRegister_(CFO_Register_DMATransferLength);
	data = data & 0x0000FFFF;
	dmaSize_ = static_cast<uint16_t>(data);
	return dmaSize_;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDMATransferLength()
{
	auto form = CreateFormatter(CFO_Register_DMATransferLength);
	form.description = "DMA Transfer Length";
	form.vals.push_back(""); //translation
	std::stringstream o;
	o << "Trigger Length: 0x" << std::hex << ReadTriggerDMATransferLength();
	form.vals.push_back(o.str());
	std::stringstream p;
	p << "Minimum Length: 0x" << std::hex << ReadMinDMATransferLength();
	form.vals.push_back(p.str());
	return form;
}

// SERDES Loopback Enable Register
void CFOLib::CFO_Registers::SetSERDESLoopbackMode(const CFO_Link_ID& link, const DTC_SERDESLoopbackMode& mode)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_SERDESLoopbackEnable);
	std::bitset<3> modeSet = mode;
	data[3 * link] = modeSet[0];
	data[3 * link + 1] = modeSet[1];
	data[3 * link + 2] = modeSet[2];
	WriteRegister_(data.to_ulong(), CFO_Register_SERDESLoopbackEnable);
}

DTCLib::DTC_SERDESLoopbackMode CFOLib::CFO_Registers::ReadSERDESLoopback(const CFO_Link_ID& link)
{
	std::bitset<3> dataSet = ReadRegister_(CFO_Register_SERDESLoopbackEnable) >> (3 * link);
	return static_cast<DTC_SERDESLoopbackMode>(dataSet.to_ulong());
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESLoopbackEnable()
{
	auto form = CreateFormatter(CFO_Register_SERDESLoopbackEnable);
	form.description = "SERDES Loopback Enable";
	form.vals.push_back(""); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": " +
							DTC_SERDESLoopbackModeConverter(ReadSERDESLoopback(r)).toString());
	}
	return form;
}

// Clock Status Register
bool CFOLib::CFO_Registers::ReadSERDESOscillatorIICError()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_ClockOscillatorStatus);
	return dataSet[2];
}

bool CFOLib::CFO_Registers::ReadSERDESOscillatorInitializationComplete()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_ClockOscillatorStatus);
	return dataSet[1];
}

bool CFOLib::CFO_Registers::WaitForSERDESOscillatorInitializationComplete(double max_wait)
{
	auto start_time = std::chrono::steady_clock::now();
	while (
		!ReadSERDESOscillatorInitializationComplete() &&
		std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start_time).count() <
			max_wait)
	{
		usleep(1000);
	}
	return ReadSERDESOscillatorInitializationComplete();
}

bool CFOLib::CFO_Registers::ReadTimingClockPLLLocked()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_ClockOscillatorStatus);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatClockOscillatorStatus()
{
	auto form = CreateFormatter(CFO_Register_ClockOscillatorStatus);
	form.description = "Clock Oscillator Status";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("SERDES IIC Error:      [") + (ReadSERDESOscillatorIICError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SERDES Init.Complete:  [") +
						(ReadSERDESOscillatorInitializationComplete() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Timing Clock PLL Lock: [") + (ReadTimingClockPLLLocked() ? "x" : " ") + "]");
	return form;
}

// Link Enable Register
void CFOLib::CFO_Registers::EnableLink(const CFO_Link_ID& link, const DTC_LinkEnableMode& mode,
									   const uint8_t& dtcCount)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_LinkEnable);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
		{
			data[i] = mode.TransmitEnable;
			data[i + 8] = mode.ReceiveEnable;
		}
	}
	else
	{
		data[link] = mode.TransmitEnable;
		data[link + 8] = mode.ReceiveEnable;
	}
	WriteRegister_(data.to_ulong(), CFO_Register_LinkEnable);
	SetMaxDTCNumber(link, dtcCount);
}

void CFOLib::CFO_Registers::DisableLink(const CFO_Link_ID& link, const DTC_LinkEnableMode& mode)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_LinkEnable);
	data[link] = data[link] && !mode.TransmitEnable;
	data[link + 8] = data[link + 8] && !mode.ReceiveEnable;
	WriteRegister_(data.to_ulong(), CFO_Register_LinkEnable);
}

DTCLib::DTC_LinkEnableMode CFOLib::CFO_Registers::ReadLinkEnabled(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_LinkEnable);
	return DTC_LinkEnableMode(dataSet[link], dataSet[link + 8]);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatLinkEnable()
{
	auto form = CreateFormatter(CFO_Register_LinkEnable);
	form.description = "Link Enable";
	form.vals.push_back("       ([TX, RX, Timing])");
	for (auto r : CFO_Links)
	{
		auto re = ReadLinkEnabled(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.TransmitEnable ? "x" : " ") + "," +
							(re.ReceiveEnable ? "x" : " ") + "]");
	}
	return form;
}

// SERDES Reset Register
void CFOLib::CFO_Registers::ResetSERDES(const CFO_Link_ID& link, int interval)
{
	CFO_TLOG(TLVL_SERDESReset) << "Entering SERDES Reset Loop for Link " << link;
	std::bitset<32> data = ReadRegister_(CFO_Register_SERDESReset);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
			data[i] = 1;
	}
	else
		data[link] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_SERDESReset);

	// usleep(interval);

	data = ReadRegister_(CFO_Register_SERDESReset);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
			data[i] = 0;
	}
	else
		data[link] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_SERDESReset);


	auto resetDone = false;
	uint32_t loops = 0;
	while (!resetDone && ++loops < 100) 
	{
		usleep(interval);		

		if(link == CFO_Link_ALL)
		{	
			uint32_t readData = ReadRegister_(CFO_Register_SERDESResetDone);
			resetDone = ((readData & 0xFF) == 0xFF);
		}
		else
			resetDone = ReadResetSERDESDone(link);
		CFO_TLOG(TLVL_SERDESReset) << "End of SERDES Reset loop=" << loops << ", done=" << std::boolalpha << resetDone;
	}
	if(loops >= 100)
	{
		CFO_TLOG(TLVL_ERROR) << "Timeout waiting for SERDES Reset loop=" << loops;
		throw DTC_IOErrorException("Timeout waiting for SERDES Reset loop.");
	}
}

void CFOLib::CFO_Registers::ResetAllSERDESPlls()
{
	WriteRegister_(0x0000ff00, CFO_Register_SERDESReset);
	WriteRegister_(0x0, CFO_Register_SERDESReset);
	sleep(3);
}
void CFOLib::CFO_Registers::ResetAllSERDESTx()
{
	WriteRegister_(0x00ff0000, CFO_Register_SERDESReset);
	WriteRegister_(0x0, CFO_Register_SERDESReset);
	sleep(3);
}

bool CFOLib::CFO_Registers::ReadResetSERDES(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_SERDESReset);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESReset()
{
	auto form = CreateFormatter(CFO_Register_SERDESReset);
	form.description = "SERDES Reset";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadResetSERDES(r) ? "x" : " ") + "]");
	}
	return form;
}

// SERDES RX Disparity Error Register
DTCLib::DTC_SERDESRXDisparityError CFOLib::CFO_Registers::ReadSERDESRXDisparityError(const CFO_Link_ID& link)
{
	return DTC_SERDESRXDisparityError(ReadRegister_(CFO_Register_SERDESRXDisparityError), static_cast<DTC_Link_ID>(link));
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESRXDisparityError()
{
	auto form = CreateFormatter(CFO_Register_SERDESRXDisparityError);
	form.description = "SERDES RX Disparity Error";
	form.vals.push_back("       ([H,L])");
	for (auto r : CFO_Links)
	{
		auto re = ReadSERDESRXDisparityError(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + std::to_string(re.GetData()[1]) + "," +
							std::to_string(re.GetData()[0]) + "]");
	}
	return form;
}

// SERDES RX Character Not In Table Error Register
DTCLib::DTC_CharacterNotInTableError CFOLib::CFO_Registers::ReadSERDESRXCharacterNotInTableError(
	const CFO_Link_ID& link)
{
	return DTC_CharacterNotInTableError(ReadRegister_(CFO_Register_SERDESRXCharacterNotInTableError),
										static_cast<DTC_Link_ID>(link));
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESRXCharacterNotInTableError()
{
	auto form = CreateFormatter(CFO_Register_SERDESRXCharacterNotInTableError);
	form.description = "SERDES RX CNIT Error";
	form.vals.push_back("       ([H,L])");
	for (auto r : CFO_Links)
	{
		auto re = ReadSERDESRXCharacterNotInTableError(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + std::to_string(re.GetData()[1]) + "," +
							std::to_string(re.GetData()[0]) + "]");
	}
	return form;
}

// SERDES Unlock Error Register
bool CFOLib::CFO_Registers::ReadSERDESUnlockError(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_SERDESUnlockError);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESUnlockError()
{
	auto form = CreateFormatter(CFO_Register_SERDESUnlockError);
	form.description = "SERDES Unlock Error";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESUnlockError(r) ? "x" : " ") +
							"]");
	}
	return form;
}

// SERDES PLL Locked Register
bool CFOLib::CFO_Registers::ReadSERDESPLLLocked(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_SERDESPLLLocked);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPLLLocked()
{
	auto form = CreateFormatter(CFO_Register_SERDESPLLLocked);
	form.description = "SERDES PLL Locked";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESPLLLocked(r) ? "x" : " ") + "]");
	}
	return form;
}

// SERDES RX Status Register
DTCLib::DTC_RXStatus CFOLib::CFO_Registers::ReadSERDESRXStatus(const CFO_Link_ID& link)
{
	auto data = ReadRegister_(CFO_Register_SERDESRXStatus);
	data = (data >> (3 * link)) & 0x7;
	return static_cast<DTC_RXStatus>(data);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESRXStatus()
{
	auto form = CreateFormatter(CFO_Register_SERDESRXStatus);
	form.description = "SERDES RX Status";
	form.vals.push_back(""); //translation
	for (auto r : CFO_Links)
	{
		auto re = ReadSERDESRXStatus(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": " + DTC_RXStatusConverter(re).toString());
	}

	return form;
}

// SERDES Reset Done Register
bool CFOLib::CFO_Registers::ReadResetSERDESDone(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_SERDESResetDone);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESResetDone()
{
	auto form = CreateFormatter(CFO_Register_SERDESResetDone);
	form.description = "SERDES Reset Done";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadResetSERDESDone(r) ? "x" : " ") + "]");
	}
	return form;
}

// SFP / SERDES Status Register

bool CFOLib::CFO_Registers::ReadSERDESRXCDRLock(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_SFPSERDESStatus);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESRXCDRLock()
{
	auto form = CreateFormatter(CFO_Register_SFPSERDESStatus);
	form.description = "SERDES CDR Lock";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESRXCDRLock(r) ? "x" : " ") + "]");
	}
	return form;
}

void CFOLib::CFO_Registers::SetBeamOnTimerPreset(uint32_t preset)
{
	WriteRegister_(preset, CFO_Register_BeamOnTimerPreset);
}

uint32_t CFOLib::CFO_Registers::ReadBeamOnTimerPreset() { return ReadRegister_(CFO_Register_BeamOnTimerPreset); }

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatBeamOnTimerPreset()
{
	auto form = CreateFormatter(CFO_Register_BeamOnTimerPreset);
	form.description = "Beam On Timer Preset Register";
	form.vals.push_back(std::to_string(ReadBeamOnTimerPreset()));
	return form;
}

void CFOLib::CFO_Registers::EnableBeamOnMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOnMode);
	data[link] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_EnableBeamOnMode);
}

void CFOLib::CFO_Registers::DisableBeamOnMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOnMode);
	data[link] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_EnableBeamOnMode);
}

bool CFOLib::CFO_Registers::ReadBeamOnMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOnMode);
	return data[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatBeamOnMode()
{
	auto form = CreateFormatter(CFO_Register_EnableBeamOnMode);
	form.description = "Enable Beam On Mode Register";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadBeamOnMode(r) ? "x" : " ") + "]");
	}
	return form;
}

void CFOLib::CFO_Registers::EnableBeamOffMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOffMode);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
			data[i] = 1;
	}
	else
		data[link] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_EnableBeamOffMode);
}

void CFOLib::CFO_Registers::DisableBeamOffMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOffMode);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
			data[i] = 0;
	}
	else
		data[link] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_EnableBeamOffMode);
}

bool CFOLib::CFO_Registers::ReadBeamOffMode(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EnableBeamOffMode);
	return data[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatBeamOffMode()
{
	auto form = CreateFormatter(CFO_Register_EnableBeamOffMode);
	form.description = "Enable Beam Off Mode Register";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadBeamOffMode(r) ? "x" : " ") + "]");
	}
	return form;
}

void CFOLib::CFO_Registers::SetClockMarkerIntervalCount(uint32_t data)
{
	WriteRegister_(data, CFO_Register_ClockMarkerIntervalCount);
}

uint32_t CFOLib::CFO_Registers::ReadClockMarkerIntervalCount()
{
	return ReadRegister_(CFO_Register_ClockMarkerIntervalCount);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatClockMarkerIntervalCount()
{
	auto form = CreateFormatter(CFO_Register_ClockMarkerIntervalCount);
	form.description = "40 MHz Clock Marker Interval Count Register";
	form.vals.push_back(std::to_string(ReadClockMarkerIntervalCount()));
	return form;
}

// SEREDES Oscillator Registers
uint32_t CFOLib::CFO_Registers::ReadSERDESOscillatorFrequency()
{
	return ReadRegister_(CFO_Register_SERDESOscillatorFrequency);
}
void CFOLib::CFO_Registers::SetSERDESOscillatorFrequency(uint32_t freq)
{
	WriteRegister_(freq, CFO_Register_SERDESOscillatorFrequency);
}
bool CFOLib::CFO_Registers::ReadSERDESOscillatorIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(CFO_Register_SERDESClock_IICBusControl));
	return dataSet[31];
}

void CFOLib::CFO_Registers::ResetSERDESOscillatorIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), CFO_Register_SERDESClock_IICBusControl);
	while (ReadSERDESOscillatorIICInterfaceReset())
	{
		usleep(1000);
	}
}

void CFOLib::CFO_Registers::WriteSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, CFO_Register_SERDESClock_IICBusLow);
	WriteRegister_(0x1, CFO_Register_SERDESClock_IICBusHigh);
	while (ReadRegister_(CFO_Register_SERDESClock_IICBusHigh) == 0x1)
	{
		usleep(1000);
	}
}

uint8_t CFOLib::CFO_Registers::ReadSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, CFO_Register_SERDESClock_IICBusLow);
	WriteRegister_(0x2, CFO_Register_SERDESClock_IICBusHigh);
	while (ReadRegister_(CFO_Register_SERDESClock_IICBusHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(CFO_Register_SERDESClock_IICBusLow);
	return static_cast<uint8_t>(data);
}


// Jitter Attenuator CSR Register
/// <summary>
/// Read the value of the Jitter Attenuator Select
/// </summary>
/// <returns>Jitter Attenuator Select value</returns>
std::bitset<2> CFOLib::CFO_Registers::ReadJitterAttenuatorSelect()
{
	std::bitset<32> data = ReadRegister_(CFO_Register_JitterAttenuatorCSR);
	std::bitset<2> output;
	output[0] = data[4];
	output[1] = data[5];
	return output;
}
/// <summary>
/// Set the Jitter Attenuator Select bits
/// </summary>
/// <param name="data">Value to set</param>
void CFOLib::CFO_Registers::SetJitterAttenuatorSelect(std::bitset<2> data)
{
	CFO_TLOG(TLVL_DEBUG) << "JA select " << data << " = " <<
		(data == 0? "Local oscillator":(data == 1? "RTF copper clock": "undefined source!"));

	std::bitset<32> regdata = ReadRegister_(CFO_Register_JitterAttenuatorCSR);
	// detection if already locked may not work
	// if(regdata[8] == 0 && regdata[4] == data[0] && regdata[5] == data[1])
	// {
	// 	CFO_TLOG(TLVL_DEBUG) << "JA already locked with selected input " << data;
	// 	return;
	// }
	//For CFO - 0 ==> Local oscillator
	//For CFO - 1 ==> RTF copper clock
	regdata[4] = data[0];
	regdata[5] = data[1];
	WriteRegister_(regdata.to_ulong(), CFO_Register_JitterAttenuatorCSR);

	//now reset the JA a la CFOLib::CFO_Registers::ResetJitterAttenuator()
	
	regdata[0] = 1;
	WriteRegister_(regdata.to_ulong(), CFO_Register_JitterAttenuatorCSR);
	usleep(1000);
	regdata[0] = 0;
	WriteRegister_(regdata.to_ulong(), CFO_Register_JitterAttenuatorCSR);	
	
	sleep(1);

	ConfigureJitterAttenuator();
	CFO_TLOG(TLVL_DEBUG) << "JA select done for input " << data;
}

/// <summary>
/// Read the Jitter Attenuator Reset bit
/// </summary>
/// <returns>Value of the Jitter Attenuator Reset bit</returns>
bool CFOLib::CFO_Registers::ReadJitterAttenuatorReset()
{
	std::bitset<32> regdata = ReadRegister_(CFO_Register_JitterAttenuatorCSR);
	return regdata[0];
}
/// <summary>
/// Reset the Jitter Attenuator
/// </summary>
void CFOLib::CFO_Registers::ResetJitterAttenuator()
{
	std::bitset<32> regdata = ReadRegister_(CFO_Register_JitterAttenuatorCSR);
	regdata[0] = 1;
	WriteRegister_(regdata.to_ulong(), CFO_Register_JitterAttenuatorCSR);
	usleep(1000);
	regdata[0] = 0;
	WriteRegister_(regdata.to_ulong(), CFO_Register_JitterAttenuatorCSR);
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>DTC_RegisterFormatter object containing register information</returns>
DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatJitterAttenuatorCSR()
{
	auto form = CreateFormatter(CFO_Register_JitterAttenuatorCSR);
	std::bitset<32> data = form.value;//ReadRegister_(CFO_Register_JitterAttenuatorCSR);
	std::bitset<2> JAinputSelect;
	JAinputSelect[0] = data[4];
	JAinputSelect[1] = data[5];
	form.description = "Jitter Attenuator CSR";
	form.vals.push_back("<field> : [<value>]"); //first value describes format
	form.vals.push_back(std::string("JA Input Select: [") + 
		(JAinputSelect.to_ulong() == 0 ? "Local oscillator"
	             : (JAinputSelect.to_ulong() == 1 ? "RTF Copper Clock"
	                        : "Undefined")) + "]");	
	form.vals.push_back(std::string("JA in Reset:   [") + (data[0] ? "YES" : "No") + "]");
	form.vals.push_back(std::string("JA Loss-of-Lock:   [") + (data[8] ? "Not Locked" : "LOCKED") + "]");
	form.vals.push_back(std::string("JA Input-0 Local oscillator:   [") + (data[9] ? "Missing" : "OK") + "]");
	form.vals.push_back(std::string("JA Input-1 RTF Copper Clock:   [") + (data[10] ? "Missing" : "OK") + "]");
	// form.vals.push_back(std::string("JA Input-2 Undefined:   [") + (data[11] ? "Missing" : "OK") + "]");
	return form;
}

uint64_t CFOLib::CFO_Registers::ReadSERDESOscillatorParameters()
{
	uint64_t data = (static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 7)) << 40) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 8)) << 32) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 9)) << 24) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 10)) << 16) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 11)) << 8) +
					static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 12));
	return data;
}
void CFOLib::CFO_Registers::SetSERDESOscillatorParameters(uint64_t program)
{
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 0x89, 0x10);

	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 7, static_cast<uint8_t>(program >> 40));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 8, static_cast<uint8_t>(program >> 32));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 9, static_cast<uint8_t>(program >> 24));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 10, static_cast<uint8_t>(program >> 16));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 11, static_cast<uint8_t>(program >> 8));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 12, static_cast<uint8_t>(program));

	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 0x89, 0);
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 0x87, 0x40);
}

DTCLib::DTC_SerdesClockSpeed CFOLib::CFO_Registers::ReadSERDESOscillatorClock()
{
	auto freq = ReadSERDESOscillatorFrequency();

	// Clocks should be accurate to 30 ppm
	if (freq > 156250000 - 4687.5 && freq < 156250000 + 4687.5) return DTC_SerdesClockSpeed_3125Gbps;
	if (freq > 125000000 - 3750 && freq < 125000000 + 3750) return DTC_SerdesClockSpeed_25Gbps;
	return DTC_SerdesClockSpeed_Unknown;
}
void CFOLib::CFO_Registers::SetSERDESOscillatorClock(DTC_SerdesClockSpeed speed)
{
	double targetFreq;
	switch (speed)
	{
		case DTC_SerdesClockSpeed_25Gbps:
			targetFreq = 125000000.0;
			break;
		case DTC_SerdesClockSpeed_3125Gbps:
			targetFreq = 156250000.0;
			break;
		case DTC_SerdesClockSpeed_48Gbps:
			targetFreq = 240000000.0;
			break;
		default:
			targetFreq = 0.0;
			break;
	}
	if (SetNewOscillatorFrequency(targetFreq))
	{
		for (auto& Link : CFO_Links)
		{
			ResetSERDES(Link, 1000);
		}
	}
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESOscillatorFrequency()
{
	auto form = CreateFormatter(CFO_Register_SERDESOscillatorFrequency);
	form.description = "SERDES Oscillator Frequency";
	std::stringstream o;
	o << std::dec << ReadSERDESOscillatorFrequency();
	form.vals.push_back(o.str());
	return form;
}
DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESOscillatorControl()
{
	auto form = CreateFormatter(CFO_Register_SERDESClock_IICBusControl);
	form.description = "SERDES Oscillator IIC Bus Control";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("Reset:  [") + (ReadSERDESOscillatorIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESOscillatorParameterLow()
{
	auto form = CreateFormatter(CFO_Register_SERDESClock_IICBusLow);
	form.description = "SERDES Oscillator IIC Bus Low";
	form.vals.push_back(""); //translation
	auto data = ReadRegister_(CFO_Register_SERDESClock_IICBusLow);
	std::ostringstream s1, s2, s3, s4;
	s1 << "Device:     " << std::showbase << std::hex << ((data & 0xFF000000) >> 24);
	form.vals.push_back(s1.str());
	s2 << "Address:    " << std::showbase << std::hex << ((data & 0xFF0000) >> 16);
	form.vals.push_back(s2.str());
	s3 << "Write Data: " << std::showbase << std::hex << ((data & 0xFF00) >> 8);
	form.vals.push_back(s3.str());
	s4 << "Read Data:  " << std::showbase << std::hex << (data & 0xFF);
	form.vals.push_back(s4.str());
	return form;
}
DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESOscillatorParameterHigh()
{
	auto form = CreateFormatter(CFO_Register_SERDESClock_IICBusHigh);
	form.description = "SERDES Oscillator IIC Bus High";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(CFO_Register_SERDESClock_IICBusHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(CFO_Register_SERDESClock_IICBusHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// Timestamp Preset Registers
void CFOLib::CFO_Registers::SetEventWindowTagPreset(const DTC_EventWindowTag& preset)
{
	auto timestamp = preset.GetEventWindowTag();
	auto timestampLow = static_cast<uint32_t>(timestamp.to_ulong());
	timestamp >>= 32;
	auto timestampHigh = static_cast<uint16_t>(timestamp.to_ulong());

	WriteRegister_(timestampLow, CFO_Register_TimestampPreset0);
	WriteRegister_(timestampHigh, CFO_Register_TimestampPreset1);
}

DTCLib::DTC_EventWindowTag CFOLib::CFO_Registers::ReadTimestampPreset()
{
	auto timestampLow = ReadRegister_(CFO_Register_TimestampPreset0);
	DTC_EventWindowTag output;
	output.SetEventWindowTag(timestampLow, static_cast<uint16_t>(ReadRegister_(CFO_Register_TimestampPreset1)));
	return output;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTimestampPreset0()
{
	auto form = CreateFormatter(CFO_Register_TimestampPreset0);
	form.description = "Timestamp Preset 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(CFO_Register_TimestampPreset0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTimestampPreset1()
{
	auto form = CreateFormatter(CFO_Register_TimestampPreset1);
	form.description = "Timestamp Preset 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(CFO_Register_TimestampPreset1);
	form.vals.push_back(o.str());
	return form;
}

// NUMDTCs Register
void CFOLib::CFO_Registers::SetMaxDTCNumber(const CFO_Link_ID& link, const uint8_t& dtcCount)
{
	ReadLinkDTCCount(link, false);
	if(link == CFO_Link_ALL)
	{	
		for(uint8_t i=0;i<8;++i)
		{
			uint32_t mask = ~(0xF << (i * 4));
			maxDTCs_ = (maxDTCs_ & mask) + ((dtcCount & 0xF) << (i * 4));
		}
	}
	else
	{
		uint32_t mask = ~(0xF << (link * 4));
		maxDTCs_ = (maxDTCs_ & mask) + ((dtcCount & 0xF) << (link * 4));
	}
	WriteRegister_(maxDTCs_, CFO_Register_NUMDTCs);
}

uint8_t CFOLib::CFO_Registers::ReadLinkDTCCount(const CFO_Link_ID& link, bool local)
{
	if (!local)
	{
		auto data = ReadRegister_(CFO_Register_NUMDTCs);
		maxDTCs_ = data;
	}
	return (maxDTCs_ >> (link * 4)) & 0xF;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatNUMDTCs()
{
	auto form = CreateFormatter(CFO_Register_NUMDTCs);
	form.description = "Number of DTCs Register";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadLinkDTCCount(r, false) ? "x" : " ") +
							"]");
	}
	return form;
}

// FIFO Full Error Flags Registers
void CFOLib::CFO_Registers::ClearFIFOFullErrorFlags(const CFO_Link_ID& link)
{
	auto flags = ReadFIFOFullErrorFlags(link);
	std::bitset<32> data0 = 0;

	data0[link] = flags.CFOLinkInput;

	WriteRegister_(data0.to_ulong(), CFO_Register_FIFOFullErrorFlag0);
}

DTCLib::DTC_FIFOFullErrorFlags CFOLib::CFO_Registers::ReadFIFOFullErrorFlags(const CFO_Link_ID& link)
{
	std::bitset<32> data0 = ReadRegister_(CFO_Register_FIFOFullErrorFlag0);
	DTC_FIFOFullErrorFlags flags;

	flags.CFOLinkInput = data0[link];

	return flags;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatFIFOFullErrorFlag0()
{
	auto form = CreateFormatter(CFO_Register_FIFOFullErrorFlag0);
	form.description = "FIFO Full Error Flags 0";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		auto re = ReadFIFOFullErrorFlags(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.CFOLinkInput ? "x" : " ") + "]");
	}
	return form;
}

// Receive Packet Error Register
void CFOLib::CFO_Registers::ClearRXElasticBufferUnderrun(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	data[static_cast<int>(link) + 24] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_ReceivePacketError);
}

bool CFOLib::CFO_Registers::ReadRXElasticBufferUnderrun(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	return data[static_cast<int>(link) + 24];
}

void CFOLib::CFO_Registers::ClearRXElasticBufferOverrun(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	data[static_cast<int>(link) + 16] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_ReceivePacketError);
}

bool CFOLib::CFO_Registers::ReadRXElasticBufferOverrun(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	return data[static_cast<int>(link) + 16];
}

void CFOLib::CFO_Registers::ClearPacketError(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	data[static_cast<int>(link) + 8] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_ReceivePacketError);
}

bool CFOLib::CFO_Registers::ReadPacketError(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	return data[static_cast<int>(link) + 8];
}

void CFOLib::CFO_Registers::ClearPacketCRCError(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	data[static_cast<int>(link)] = 0;
	WriteRegister_(data.to_ulong(), CFO_Register_ReceivePacketError);
}

bool CFOLib::CFO_Registers::ReadPacketCRCError(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_ReceivePacketError);
	return data[static_cast<int>(link)];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketError()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketError);
	form.description = "Receive Packet Error";
	form.vals.push_back("       ([CRC, PacketError, RX Overrun, RX Underrun])");
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadPacketCRCError(r) ? "x" : " ") + "," +
							(ReadPacketError(r) ? "x" : " ") + "," + (ReadRXElasticBufferOverrun(r) ? "x" : " ") + "," +
							(ReadRXElasticBufferUnderrun(r) ? "x" : " ") + "]");
	}
	return form;
}

void CFOLib::CFO_Registers::SetEventWindowEmulatorInterval(const uint32_t& data)
{
	WriteRegister_(data, CFO_Register_EventWindowEmulatorIntervalTime);
}

uint32_t CFOLib::CFO_Registers::ReadEventWindowEmulatorInterval()
{
	return ReadRegister_(CFO_Register_EventWindowEmulatorIntervalTime);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatEventWindowEmulatorIntervalTime()
{
	auto form = CreateFormatter(CFO_Register_EventWindowEmulatorIntervalTime);
	form.description = "Event Window Emulator Interval Time";
	form.vals.push_back(std::to_string(ReadEventWindowEmulatorInterval()));
	return form;
}

void CFOLib::CFO_Registers::SetEventWindowHoldoffTime(const uint32_t& data)
{
	WriteRegister_(data, CFO_Register_EventWindowHoldoffTime);
}

uint32_t CFOLib::CFO_Registers::ReadEventWindowHoldoffTime()
{
	return ReadRegister_(CFO_Register_EventWindowHoldoffTime);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatEventWindowHoldoffTime()
{
	auto form = CreateFormatter(CFO_Register_EventWindowHoldoffTime);
	form.description = "Event Window Holdoff Time";
	form.vals.push_back(std::to_string(ReadEventWindowHoldoffTime()));
	return form;
}

bool CFOLib::CFO_Registers::ReadEventWindowTimeoutError(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_EventWindowTimeoutError);
	return dataSet[link];
}

void CFOLib::CFO_Registers::ClearEventWindowTimeoutError(const CFO_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(CFO_Register_EventWindowTimeoutError);
	data[link] = 1;
	WriteRegister_(data.to_ulong(), CFO_Register_EventWindowTimeoutError);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatEventWindowTimeoutError()
{
	auto form = CreateFormatter(CFO_Register_EventWindowTimeoutError);
	form.description = "Event Window Timeout Error";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" +
							(ReadEventWindowTimeoutError(r) ? "x" : " ") + "]");
	}
	return form;
}

void CFOLib::CFO_Registers::SetEventWindowTimeoutInterval(const uint32_t& data)
{
	WriteRegister_(data, CFO_Register_EventWindowTimeoutValue);
}

uint32_t CFOLib::CFO_Registers::ReadEventWindowTimeoutInterval()
{
	return ReadRegister_(CFO_Register_EventWindowTimeoutValue);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatEventWindowTimeoutInterval()
{
	auto form = CreateFormatter(CFO_Register_EventWindowTimeoutValue);
	form.description = "Event Window Timeout Value";
	form.vals.push_back(std::to_string(ReadEventWindowTimeoutInterval()));
	return form;
}

// SERDES Counter Registers
void CFOLib::CFO_Registers::ClearReceiveByteCount(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_ReceiveByteCountDataLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_ReceiveByteCountDataLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_ReceiveByteCountDataLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_ReceiveByteCountDataLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_ReceiveByteCountDataLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_ReceiveByteCountDataLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_ReceiveByteCountDataLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_ReceiveByteCountDataLink7;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t CFOLib::CFO_Registers::ReadReceiveByteCount(const CFO_Link_ID& link)
{
	switch (link)
	{
		case CFO_Link_0:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink0);
		case CFO_Link_1:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink1);
		case CFO_Link_2:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink2);
		case CFO_Link_3:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink3);
		case CFO_Link_4:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink4);
		case CFO_Link_5:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink5);
		case CFO_Link_6:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink6);
		case CFO_Link_7:
			return ReadRegister_(CFO_Register_ReceiveByteCountDataLink7);
		default:
			return 0;
	}
}

void CFOLib::CFO_Registers::ClearReceivePacketCount(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_ReceivePacketCountDataLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_ReceivePacketCountDataLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_ReceivePacketCountDataLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_ReceivePacketCountDataLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_ReceivePacketCountDataLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_ReceivePacketCountDataLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_ReceivePacketCountDataLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_ReceivePacketCountDataLink7;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t CFOLib::CFO_Registers::ReadReceivePacketCount(const CFO_Link_ID& link)
{
	switch (link)
	{
		case CFO_Link_0:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink0);
		case CFO_Link_1:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink1);
		case CFO_Link_2:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink2);
		case CFO_Link_3:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink3);
		case CFO_Link_4:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink4);
		case CFO_Link_5:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink5);
		case CFO_Link_6:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink6);
		case CFO_Link_7:
			return ReadRegister_(CFO_Register_ReceivePacketCountDataLink7);
		default:
			return 0;
	}
}

void CFOLib::CFO_Registers::ClearTransmitByteCount(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_TransmitByteCountDataLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_TransmitByteCountDataLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_TransmitByteCountDataLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_TransmitByteCountDataLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_TransmitByteCountDataLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_TransmitByteCountDataLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_TransmitByteCountDataLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_TransmitByteCountDataLink7;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t CFOLib::CFO_Registers::ReadTransmitByteCount(const CFO_Link_ID& link)
{
	switch (link)
	{
		case CFO_Link_0:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink0);
		case CFO_Link_1:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink1);
		case CFO_Link_2:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink2);
		case CFO_Link_3:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink3);
		case CFO_Link_4:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink4);
		case CFO_Link_5:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink5);
		case CFO_Link_6:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink6);
		case CFO_Link_7:
			return ReadRegister_(CFO_Register_TransmitByteCountDataLink7);
		default:
			return 0;
	}
}

void CFOLib::CFO_Registers::ClearTransmitPacketCount(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_TransmitPacketCountDataLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_TransmitPacketCountDataLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_TransmitPacketCountDataLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_TransmitPacketCountDataLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_TransmitPacketCountDataLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_TransmitPacketCountDataLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_TransmitPacketCountDataLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_TransmitPacketCountDataLink7;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t CFOLib::CFO_Registers::ReadTransmitPacketCount(const CFO_Link_ID& link)
{
	switch (link)
	{
		case CFO_Link_0:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink0);
		case CFO_Link_1:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink1);
		case CFO_Link_2:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink2);
		case CFO_Link_3:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink3);
		case CFO_Link_4:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink4);
		case CFO_Link_5:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink5);
		case CFO_Link_6:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink6);
		case CFO_Link_7:
			return ReadRegister_(CFO_Register_TransmitPacketCountDataLink7);
		default:
			return 0;
	}
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink0()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink0);
	form.description = "Receive Byte Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink1()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink1);
	form.description = "Receive Byte Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink2()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink2);
	form.description = "Receive Byte Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink3()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink3);
	form.description = "Receive Byte Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink4()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink4);
	form.description = "Receive Byte Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink5()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink5);
	form.description = "Receive Byte Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink6()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink6);
	form.description = "Receive Byte Count: Link 6";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_6);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceiveByteCountLink7()
{
	auto form = CreateFormatter(CFO_Register_ReceiveByteCountDataLink7);
	form.description = "Receive Byte Count: Link 7";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(CFO_Link_7);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink0()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink0);
	form.description = "Receive Packet Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink1()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink1);
	form.description = "Receive Packet Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink2()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink2);
	form.description = "Receive Packet Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink3()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink3);
	form.description = "Receive Packet Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink4()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink4);
	form.description = "Receive Packet Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink5()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink5);
	form.description = "Receive Packet Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink6()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink6);
	form.description = "Receive Packet Count: Link 6";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_6);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatReceivePacketCountLink7()
{
	auto form = CreateFormatter(CFO_Register_ReceivePacketCountDataLink7);
	form.description = "Receive Packet Count: Link 7";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(CFO_Link_7);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink0()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink0);
	form.description = "Transmit Byte Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink1()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink1);
	form.description = "Transmit Byte Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink2()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink2);
	form.description = "Transmit Byte Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink3()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink3);
	form.description = "Transmit Byte Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink4()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink4);
	form.description = "Transmit Byte Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink5()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink5);
	form.description = "Transmit Byte Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink6()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink6);
	form.description = "Transmit Byte Count: Link 6";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_6);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTramsitByteCountLink7()
{
	auto form = CreateFormatter(CFO_Register_TransmitByteCountDataLink7);
	form.description = "Transmit Byte Count: Link 7";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(CFO_Link_7);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink0()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink0);
	form.description = "Transmit Packet Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink1()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink1);
	form.description = "Transmit Packet Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink2()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink2);
	form.description = "Transmit Packet Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink3()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink3);
	form.description = "Transmit Packet Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink4()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink4);
	form.description = "Transmit Packet Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink5()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink5);
	form.description = "Transmit Packet Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink6()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink6);
	form.description = "Transmit Packet Count: Link 6";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_6);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatTransmitPacketCountLink7()
{
	auto form = CreateFormatter(CFO_Register_TransmitPacketCountDataLink7);
	form.description = "Transmit Packet Count: Link 7";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(CFO_Link_7);
	form.vals.push_back(o.str());
	return form;
}

// DMA Address Registers
void CFOLib::CFO_Registers::SetDMAWriteStartAddress(const uint32_t& address)
{
	WriteRegister_(address, CFO_Register_DDRMemoryDMAWriteStartAddress);
}

uint32_t CFOLib::CFO_Registers::ReadDMAWriteStartAddress()
{
	return ReadRegister_(CFO_Register_DDRMemoryDMAWriteStartAddress);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDMAWriteStartAddress()
{
	auto form = CreateFormatter(CFO_Register_DDRMemoryDMAWriteStartAddress);
	form.description = "DDR Memory DMA Write Start Address";
	form.vals.push_back(std::to_string(ReadDMAWriteStartAddress()));
	return form;
}

void CFOLib::CFO_Registers::SetDMAReadStartAddress(const uint32_t& address)
{
	WriteRegister_(address, CFO_Register_DDRMemoryDMAReadStartAddress);
}

uint32_t CFOLib::CFO_Registers::ReadDMAReadStartAddress()
{
	return ReadRegister_(CFO_Register_DDRMemoryDMAReadStartAddress);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDMAReadStartAddress()
{
	auto form = CreateFormatter(CFO_Register_DDRMemoryDMAWriteStartAddress);
	form.description = "DDR Memory DMA Read Start Address";
	form.vals.push_back(std::to_string(ReadDMAReadStartAddress()));
	return form;
}

void CFOLib::CFO_Registers::SetDMAReadByteCount(const uint32_t& bytes)
{
	WriteRegister_(bytes, CFO_Register_DDRMemoryDMAReadByteCount);
}

uint32_t CFOLib::CFO_Registers::ReadDMAReadByteCount() { return ReadRegister_(CFO_Register_DDRMemoryDMAReadByteCount); }

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDMAReadByteCount()
{
	auto form = CreateFormatter(CFO_Register_DDRMemoryDMAWriteStartAddress);
	form.description = "DDR Memory DMA Read Byte Count/Enable";
	form.vals.push_back(std::to_string(ReadDMAReadByteCount()));
	return form;
}

void CFOLib::CFO_Registers::SetDDRBeamOnBaseAddress(const uint32_t& address)
{
	WriteRegister_(address, CFO_Register_DDRBeamOnBaseAddress);
}

uint32_t CFOLib::CFO_Registers::ReadDDRBeamOnBaseAddress() { return ReadRegister_(CFO_Register_DDRBeamOnBaseAddress); }

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDDRBeamOnBaseAddress()
{
	auto form = CreateFormatter(CFO_Register_DDRBeamOnBaseAddress);
	form.description = "DDR Memory Beam On Base Address";
	form.vals.push_back(std::to_string(ReadDDRBeamOnBaseAddress()));
	return form;
}

void CFOLib::CFO_Registers::SetDDRBeamOffBaseAddress(const uint32_t& address)
{
	WriteRegister_(address, CFO_Register_DDRBeamOffBaseAddress);
}

uint32_t CFOLib::CFO_Registers::ReadDDRBeamOffBaseAddress()
{
	return ReadRegister_(CFO_Register_DDRBeamOffBaseAddress);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatDDRBeamOffBaseAddress()
{
	auto form = CreateFormatter(CFO_Register_DDRBeamOffBaseAddress);
	form.description = "DDR Memory Beam Off Base Address";
	form.vals.push_back(std::to_string(ReadDDRBeamOffBaseAddress()));
	return form;
}

// Firefly CSR Register
bool CFOLib::CFO_Registers::ReadFireflyTXRXPresent()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[26];
}

bool CFOLib::CFO_Registers::ReadFireflyRXPresent()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[25];
}

bool CFOLib::CFO_Registers::ReadFireflyTXPresent()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[24];
}

bool CFOLib::CFO_Registers::ReadFireflyTXRXInterrupt()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[18];
}

bool CFOLib::CFO_Registers::ReadFireflyRXInterrupt()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[17];
}

bool CFOLib::CFO_Registers::ReadFireflyTXInterrupt()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[16];
}

void CFOLib::CFO_Registers::SetFireflyTXRXSelect(bool select)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[10] = select;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

void CFOLib::CFO_Registers::SetFireflyRXSelect(bool select)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[9] = select;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

void CFOLib::CFO_Registers::SetFireflyTXSelect(bool select)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[8] = select;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

bool CFOLib::CFO_Registers::ReadFireflyTXRXSelect()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[10];
}

bool CFOLib::CFO_Registers::ReadFireflyRXSelect()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[9];
}

bool CFOLib::CFO_Registers::ReadFireflyTXSelect()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[8];
}

void CFOLib::CFO_Registers::SetFireflyTXRXReset(bool reset)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[2] = reset;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

void CFOLib::CFO_Registers::SetFireflyRXReset(bool reset)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[1] = reset;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

void CFOLib::CFO_Registers::SetFireflyTXReset(bool reset)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	dataSet[0] = reset;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_FireflyCSRRegister);
}

bool CFOLib::CFO_Registers::ReadFireflyTXRXReset()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[2];
}

bool CFOLib::CFO_Registers::ReadFireflyRXReset()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[1];
}

bool CFOLib::CFO_Registers::ReadFireflyTXReset()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FireflyCSRRegister);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatFireflyCSR()
{
	auto form = CreateFormatter(CFO_Register_FireflyCSRRegister);
	form.description = "Firefly CSR Register";
	form.vals.push_back("      ([Present, Interrupt, Select, Reset])");
	form.vals.push_back(std::string("TX/RX: [") + (ReadFireflyTXRXPresent() ? "x" : " ") +
						(ReadFireflyTXRXInterrupt() ? "x" : " ") + (ReadFireflyTXRXSelect() ? "x" : " ") +
						(ReadFireflyTXRXReset() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX:    [") + (ReadFireflyRXPresent() ? "x" : " ") +
						(ReadFireflyRXInterrupt() ? "x" : " ") + (ReadFireflyRXSelect() ? "x" : " ") +
						(ReadFireflyRXReset() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX:    [") + (ReadFireflyTXPresent() ? "x" : " ") +
						(ReadFireflyTXInterrupt() ? "x" : " ") + (ReadFireflyTXSelect() ? "x" : " ") +
						(ReadFireflyTXReset() ? "x" : " ") + "]");

	return form;
}

// SERDES PRBS Control Registers
bool CFOLib::CFO_Registers::ReadSERDESPRBSErrorFlag(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet;
	switch (link)
	{
		case CFO_Link_0:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink0);
			break;
		case CFO_Link_1:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink1);
			break;
		case CFO_Link_2:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink2);
			break;
		case CFO_Link_3:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink3);
			break;
		case CFO_Link_4:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink4);
			break;
		case CFO_Link_5:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink5);
			break;
		case CFO_Link_6:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink6);
			break;
		case CFO_Link_7:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink7);
			break;
		default:
			dataSet = 0;
			break;
	}
	return dataSet[31];
}

uint8_t CFOLib::CFO_Registers::ReadSERDESTXPRBSSEL(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return 0;
	}
	auto data = ReadRegister_(reg);
	return (data >> 12) & 0xF;
}

void CFOLib::CFO_Registers::SetSERDESTXPRBSSEL(const CFO_Link_ID& link, uint8_t byte)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	WriteRegister_(byte, reg);
}

uint8_t CFOLib::CFO_Registers::ReadSERDESRXPRBSSEL(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return 0;
	}
	auto data = ReadRegister_(reg);
	return (data >> 8) & 0xF;
}

void CFOLib::CFO_Registers::SetSERDESRXPRBSSEL(const CFO_Link_ID& link, uint8_t byte)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	WriteRegister_(byte, reg);
}

bool CFOLib::CFO_Registers::ReadSERDESTXPRBSForceError(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet;
	switch (link)
	{
		case CFO_Link_0:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink0);
			break;
		case CFO_Link_1:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink1);
			break;
		case CFO_Link_2:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink2);
			break;
		case CFO_Link_3:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink3);
			break;
		case CFO_Link_4:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink4);
			break;
		case CFO_Link_5:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink5);
			break;
		case CFO_Link_6:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink6);
			break;
		case CFO_Link_7:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink7);
			break;
		default:
			dataSet = 0;
			break;
	}
	return dataSet[1];
}

void CFOLib::CFO_Registers::SetSERDESTXPRBSForceError(const CFO_Link_ID& link, bool flag)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	std::bitset<32> dataSet = ReadRegister_(reg);
	dataSet[1] = flag;
	WriteRegister_(dataSet.to_ulong(), reg);
}

void CFOLib::CFO_Registers::ToggleSERDESTXPRBSForceError(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	std::bitset<32> dataSet = ReadRegister_(reg);
	dataSet[1] = !dataSet[1];
	WriteRegister_(dataSet.to_ulong(), reg);
}

bool CFOLib::CFO_Registers::ReadSERDESRXPRBSCountReset(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet;
	switch (link)
	{
		case CFO_Link_0:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink0);
			break;
		case CFO_Link_1:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink1);
			break;
		case CFO_Link_2:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink2);
			break;
		case CFO_Link_3:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink3);
			break;
		case CFO_Link_4:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink4);
			break;
		case CFO_Link_5:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink5);
			break;
		case CFO_Link_6:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink6);
			break;
		case CFO_Link_7:
			dataSet = ReadRegister_(CFO_Register_SERDESPRBSControlLink7);
			break;
		default:
			dataSet = 0;
			break;
	}
	return dataSet[0];
}

void CFOLib::CFO_Registers::SetSERDESRXPRBSCountReset(const CFO_Link_ID& link, bool flag)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	std::bitset<32> dataSet = ReadRegister_(reg);
	dataSet[0] = flag;
	WriteRegister_(dataSet.to_ulong(), reg);
}

void CFOLib::CFO_Registers::ToggleSERDESRXPRBSCountReset(const CFO_Link_ID& link)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_SERDESPRBSControlLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_SERDESPRBSControlLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_SERDESPRBSControlLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_SERDESPRBSControlLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_SERDESPRBSControlLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_SERDESPRBSControlLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_SERDESPRBSControlLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_SERDESPRBSControlLink7;
			break;
		default:
			return;
	}
	std::bitset<32> dataSet = ReadRegister_(reg);
	dataSet[0] = !dataSet[0];
	WriteRegister_(dataSet.to_ulong(), reg);
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink0()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink0);
	form.description = "SERDES PRBS Control Link 0";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_0);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_0));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_0));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_0);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_0);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink1()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink1);
	form.description = "SERDES PRBS Control Link 1";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_1);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_1));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_1));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_1);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_1);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink2()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink2);
	form.description = "SERDES PRBS Control Link 2";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_2);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_2));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_2));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_2);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_2);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink3()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink3);
	form.description = "SERDES PRBS Control Link 3";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_3);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_3));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_3));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_3);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_3);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink4()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink4);
	form.description = "SERDES PRBS Control Link 4";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_4);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_4));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_4));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_4);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_4);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink5()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink5);
	form.description = "SERDES PRBS Control Link 5";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_5);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_5));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_5));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_5);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_5);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink6()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink6);
	form.description = "SERDES PRBS Control Link 6";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_6);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_6));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_6));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_6);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_6);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatSERDESPRBSControlLink7()
{
	auto form = CreateFormatter(CFO_Register_SERDESPRBSControlLink7);
	form.description = "SERDES PRBS Control Link 7";
	form.vals.push_back(""); //translation
	std::ostringstream o;
	o << "RX PRBS Error:              " << std::boolalpha << ReadSERDESPRBSErrorFlag(CFO_Link_7);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Select:             " << std::to_string(ReadSERDESTXPRBSSEL(CFO_Link_7));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Select:             " << std::to_string(ReadSERDESRXPRBSSEL(CFO_Link_7));
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "TX PRBS Enable Force Error: " << std::boolalpha << ReadSERDESTXPRBSForceError(CFO_Link_7);
	form.vals.push_back(o.str());
	o.flush();
	o.str("");
	o << "RX PRBS Count Reset:        " << std::boolalpha << ReadSERDESRXPRBSCountReset(CFO_Link_7);
	form.vals.push_back(o.str());
	o.flush();

	return form;
}

void CFOLib::CFO_Registers::SetCableDelayValue(const CFO_Link_ID& link, const uint32_t delay)
{
	CFO_Register reg;
	switch (link)
	{
		case CFO_Link_0:
			reg = CFO_Register_CableDelayValueLink0;
			break;
		case CFO_Link_1:
			reg = CFO_Register_CableDelayValueLink1;
			break;
		case CFO_Link_2:
			reg = CFO_Register_CableDelayValueLink2;
			break;
		case CFO_Link_3:
			reg = CFO_Register_CableDelayValueLink3;
			break;
		case CFO_Link_4:
			reg = CFO_Register_CableDelayValueLink4;
			break;
		case CFO_Link_5:
			reg = CFO_Register_CableDelayValueLink5;
			break;
		case CFO_Link_6:
			reg = CFO_Register_CableDelayValueLink6;
			break;
		case CFO_Link_7:
			reg = CFO_Register_CableDelayValueLink7;
			break;
		default:
			return;
	}
	WriteRegister_(delay, reg);
}

uint32_t CFOLib::CFO_Registers::ReadCableDelayValue(const CFO_Link_ID& link)
{
	switch (link)
	{
		case CFO_Link_0:
			return ReadRegister_(CFO_Register_CableDelayValueLink0);
		case CFO_Link_1:
			return ReadRegister_(CFO_Register_CableDelayValueLink1);
		case CFO_Link_2:
			return ReadRegister_(CFO_Register_CableDelayValueLink2);
		case CFO_Link_3:
			return ReadRegister_(CFO_Register_CableDelayValueLink3);
		case CFO_Link_4:
			return ReadRegister_(CFO_Register_CableDelayValueLink4);
		case CFO_Link_5:
			return ReadRegister_(CFO_Register_CableDelayValueLink5);
		case CFO_Link_6:
			return ReadRegister_(CFO_Register_CableDelayValueLink6);
		case CFO_Link_7:
			return ReadRegister_(CFO_Register_CableDelayValueLink7);
		default:
			return -1;
	}
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink0()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink0);
	form.description = "Cable Delay Value Link 0";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_0)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink1()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink1);
	form.description = "Cable Delay Value Link 1";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_1)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink2()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink2);
	form.description = "Cable Delay Value Link 2";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_2)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink3()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink3);
	form.description = "Cable Delay Value Link 3";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_3)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink4()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink4);
	form.description = "Cable Delay Value Link 4";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_4)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink5()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink5);
	form.description = "Cable Delay Value Link 5";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_5)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink6()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink6);
	form.description = "Cable Delay Value Link 6";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_6)));
	return form;
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayValueLink7()
{
	auto form = CreateFormatter(CFO_Register_CableDelayValueLink7);
	form.description = "Cable Delay Value Link 7";
	form.vals.push_back(std::to_string(ReadCableDelayValue(CFO_Link_7)));
	return form;
}

void CFOLib::CFO_Registers::ResetDelayRegister()
{
	WriteRegister_(0, CFO_Register_CableDelayControlStatus);
}


bool CFOLib::CFO_Registers::ReadDelayMeasureError(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	return dataSet[24 + link];
}

bool CFOLib::CFO_Registers::ReadDelayExternalLoopbackEnable()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	return dataSet[16];
}

void CFOLib::CFO_Registers::SetDelayExternalLoopbackEnable(bool value)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	dataSet[16] = value;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_CableDelayControlStatus);
}

void CFOLib::CFO_Registers::EnableDelayMeasureMode(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	dataSet[8 + link] = 1;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_CableDelayControlStatus);
}

void CFOLib::CFO_Registers::DisableDelayMeasureMode(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	dataSet[8 + link] = 0;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_CableDelayControlStatus);
}

bool CFOLib::CFO_Registers::ReadDelayMeasureMode(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	return dataSet[8 + link];
}

void CFOLib::CFO_Registers::EnableDelayMeasureNow(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	dataSet[link] = 1;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_CableDelayControlStatus);
}

void CFOLib::CFO_Registers::DisableDelayMeasureNow(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	dataSet[link] = 0;
	WriteRegister_(dataSet.to_ulong(), CFO_Register_CableDelayControlStatus);
}

bool CFOLib::CFO_Registers::ReadDelayMeasureNow(const CFO_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_CableDelayControlStatus);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatCableDelayControl()
{
	auto form = CreateFormatter(CFO_Register_CableDelayControlStatus);
	form.description = "Cabel Delay Control and Status";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("Delay External Loopback Enable: [") +
						(ReadDelayExternalLoopbackEnable() ? "x" : " ") + std::string("]"));
	form.vals.push_back(std::string("Delay Measure Flags:           ([Error, Enabled, Now])"));
	for (auto r : CFO_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ":                         [" +
							(ReadDelayMeasureError(r) ? "x" : " ") + "," + (ReadDelayMeasureMode(r) ? "x" : " ") + "," +
							(ReadDelayMeasureNow(r) ? "x" : " ") + "]");
	}
	return form;
}

// FPGA PROM Program Status Register
bool CFOLib::CFO_Registers::ReadFPGAPROMProgramFIFOFull()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FPGAPROMProgramStatus);
	return dataSet[1];
}

bool CFOLib::CFO_Registers::ReadFPGAPROMReady()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FPGAPROMProgramStatus);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatFPGAPROMProgramStatus()
{
	auto form = CreateFormatter(CFO_Register_FPGAPROMProgramStatus);
	form.description = "FPGA PROM Program Status";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("FPGA PROM Program FIFO Full: [") + (ReadFPGAPROMProgramFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("FPGA PROM Ready:             [") + (ReadFPGAPROMReady() ? "x" : " ") + "]");
	return form;
}

// FPGA Core Access Register
void CFOLib::CFO_Registers::ReloadFPGAFirmware()
{
	WriteRegister_(0xFFFFFFFF, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0xAA995566, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x20000000, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x30020001, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x00000000, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x30008001, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x0000000F, CFO_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x20000000, CFO_Register_FPGACoreAccess);
}

bool CFOLib::CFO_Registers::ReadFPGACoreAccessFIFOFull()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FPGACoreAccess);
	return dataSet[1];
}

bool CFOLib::CFO_Registers::ReadFPGACoreAccessFIFOEmpty()
{
	std::bitset<32> dataSet = ReadRegister_(CFO_Register_FPGACoreAccess);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter CFOLib::CFO_Registers::FormatFPGACoreAccess()
{
	auto form = CreateFormatter(CFO_Register_FPGACoreAccess);
	form.description = "FPGA Core Access";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("FPGA Core Access FIFO Full:  [") + (ReadFPGACoreAccessFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA Core Access FIFO Empty: [") + (ReadFPGACoreAccessFIFOEmpty() ? "x" : " ") +
						"]");

	return form;
}

// Oscillator Programming (DDR and SERDES)
bool CFOLib::CFO_Registers::SetNewOscillatorFrequency(double targetFrequency)
{
	auto currentFrequency = ReadSERDESOscillatorFrequency();
	auto currentProgram = ReadSERDESOscillatorParameters();
	CFO_TLOG(TLVL_DEBUG) << "Target Frequency: " << targetFrequency << ", Current Frequency: " << currentFrequency
						 << ", Current Program: " << std::showbase << std::hex << currentProgram;

	// Check if targetFrequency is essentially the same as the current frequency...
	if (fabs(currentFrequency - targetFrequency) < targetFrequency * 30 / 1000000)
	{
		CFO_TLOG(TLVL_INFO) << "New frequency and old frequency are within 30 ppm of each other, not reprogramming!";
		return false;
	}

	auto newParameters = CalculateFrequencyForProgramming_(targetFrequency, currentFrequency, currentProgram);
	if (newParameters == 0)
	{
		CFO_TLOG(TLVL_WARNING) << "New program calculated as 0! Check parameters!";
		return false;
	}
	SetSERDESOscillatorParameters(newParameters);
	SetSERDESOscillatorFrequency(targetFrequency);
	return true;
}

void CFOLib::CFO_Registers::DisableLinks()
{
	CFO_TLOG(TLVL_INFO) << "CFO disable serdes transmit and receive";
	WriteRegister_(0, CFO_Register_LinkEnable);
}

void CFOLib::CFO_Registers::DisableAllOutputs()
{
	CFO_TLOG(TLVL_INFO) << "CFO disable Event Start character output";
	WriteRegister_(0,CFO_Register_CFOControl);

	CFO_TLOG(TLVL_INFO) << "CFO disable serdes transmit and receive";
	WriteRegister_(0,CFO_Register_LinkEnable);

	CFO_TLOG(TLVL_INFO) << "CFO turn off Event Windows";
	WriteRegister_(0,CFO_Register_EventWindowEmulatorIntervalTime);

	CFO_TLOG(TLVL_INFO) << "CFO turn off 40MHz marker interval";
	WriteRegister_(0,CFO_Register_ClockMarkerIntervalCount);
}


// Private Functions
void CFOLib::CFO_Registers::WriteRegister_(uint32_t dataToWrite, const CFO_Register& address)
{
	auto retry = 3;
	int errorCode;
	do
	{
		errorCode = device_.write_register(address, 100, dataToWrite);
		--retry;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0)
	{
		throw DTC_IOErrorException(errorCode);
	}

	{
		std::stringstream o;
		o << device_.getDeviceUID() << " - " << 
			"write value 0x"	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(dataToWrite)
			<< " to register 0x" 	<< std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << 
			std::endl;
		CFO_TLOG(TLVL_DEBUG) << o.str();
	}

	//verify register readback
	if(1)
	{
		uint32_t readbackValue = ReadRegister_(address);
		int i = -1;  // used for counters
		switch(address) //handle special register checks by masking of DONT-CARE bits, or else check full 32 bits
		{
			//---------- TODO: check if CFO 0x9380 need a delay between write and read op.
			case CFO_Register_CableDelayControlStatus:
				usleep(100);
				return;	// do not check
			//---------- CFO and DTC registers
			case CFO_Register_SERDESClock_IICBusLow: // lowest 16-bits are the I2C read value. So ignore in write validation			
			// case 0x9298 FIXME and add CFO Firefly feature? --> DTC_Register_FireflyRX_IICBusConfigLow:
				dataToWrite		&= 0xffff0000; 
				readbackValue 	&= 0xffff0000; 
				break;
			// case 0x93a0 FIXME and add CFO Firefly feature? DTC_Register_FireFlyControlStatus: // upper 16-bits are part of I2C operation. So ignore in write validation			
				// dataToWrite		&= 0x0000ffff; 
				// readbackValue 	&= 0x0000ffff; 
				// break;
			case CFO_Register_CFOControl: //bit 31 is reset bit, which is write only 
				dataToWrite		&= 0x7fffffff;
				readbackValue   &= 0x7fffffff; 
				break;			
			case CFO_Register_SERDESClock_IICBusHigh:  // this is an I2C register, it clears bit-0 when transaction
	              // finishes
				while((dataToWrite & 0x1) && (readbackValue & 0x1))  // wait for I2C to clear...
				{
					readbackValue = ReadRegister_(address);
					usleep(100);
					if((++i % 10) == 9)
						CFO_TLOG(TLVL_DEBUG) << "I2C waited " << i + 1 << " times..." << std::endl;
				}
				dataToWrite &= ~1;
				readbackValue &= ~1;
				break;
			
			//---------- CFO only registers
			case CFO_Register_JitterAttenuatorCSR:  // 0x9500 bit-0 is reset, input select bit-5:4, bit-8 is LOL, bit-11:9
						// (input LOS).. only check input select bits
				dataToWrite &= (3 << 4);
				readbackValue &= (3 << 4);
				break;

			default:; // do direct comparison of each bit
		} //end readback verification address case handling

		if(readbackValue != dataToWrite)
		{
			std::stringstream ss;
			ss << device_.getDeviceUID() << " - " << 
					"write value 0x"	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(dataToWrite)
					<< " to register 0x" 	<< std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << 
					"... read back 0x"	 	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(readbackValue) << 
					std::endl;
			CFO_TLOG(TLVL_ERROR) << ss.str();
			throw DTC_IOErrorException(ss.str());
			// __FE_COUT_ERR__ << ss.str(); 
		}

	} //end verify register readback
}

uint32_t CFOLib::CFO_Registers::ReadRegister_(const CFO_Register& address)
{
	auto retry = 3;
	int errorCode;
	uint32_t data;
	do
	{
		errorCode = device_.read_register(address, 100, &data);
		--retry;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0)
	{
		throw DTC_IOErrorException(errorCode);
	}

	if(address != 0x916c)
	{
		std::stringstream o;
		o << device_.getDeviceUID() << " - " << 
			"read value 0x"	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(data)
			<< " from register 0x" 	<< std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << 
			std::endl;
		CFO_TLOG(TLVL_DEBUG) << o.str();
	}

	return data;
}

int CFOLib::CFO_Registers::DecodeHighSpeedDivider_(int input)
{
	switch (input)
	{
		case 0:
			return 4;
		case 1:
			return 5;
		case 2:
			return 6;
		case 3:
			return 7;
		case 5:
			return 9;
		case 7:
			return 11;
		default:
			return -1;
	}
}

int CFOLib::CFO_Registers::EncodeHighSpeedDivider_(int input)
{
	switch (input)
	{
		case 4:
			return 0;
		case 5:
			return 1;
		case 6:
			return 2;
		case 7:
			return 3;
		case 9:
			return 5;
		case 11:
			return 7;
		default:
			return -1;
	}
}

int CFOLib::CFO_Registers::EncodeOutputDivider_(int input)
{
	if (input == 1) return 0;
	int temp = input / 2;
	return (temp * 2) - 1;
}

uint64_t CFOLib::CFO_Registers::CalculateFrequencyForProgramming_(double targetFrequency, double currentFrequency,
																  uint64_t currentProgram)
{
	CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: targetFrequency=" << targetFrequency << ", currentFrequency=" << currentFrequency
				<< ", currentProgram=" << std::showbase << std::hex << static_cast<unsigned long long>(currentProgram);
	auto currentHighSpeedDivider = DecodeHighSpeedDivider_((currentProgram >> 45) & 0x7);
	auto currentOutputDivider = DecodeOutputDivider_((currentProgram >> 38) & 0x7F);
	auto currentRFREQ = DecodeRFREQ_(currentProgram & 0x3FFFFFFFFF);
	CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: Current HSDIV=" << currentHighSpeedDivider << ", N1=" << currentOutputDivider << ", RFREQ=" << currentRFREQ;
	const auto minFreq = 4850000000;  // Hz
	const auto maxFreq = 5670000000;  // Hz

	auto fXTAL = currentFrequency * currentHighSpeedDivider * currentOutputDivider / currentRFREQ;
	CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: fXTAL=" << fXTAL;

	std::vector<int> hsdiv_values = {11, 9, 7, 6, 5, 4};
	std::vector<std::pair<int, double>> parameter_values;
	for (auto hsdiv : hsdiv_values)
	{
		auto minN = minFreq / (targetFrequency * hsdiv);
		if (minN > 128) break;

		auto thisN = 2;
		if (minN < 1) thisN = 1;
		while (thisN < minN)
		{
			thisN += 2;
		}
		auto fdco_new = hsdiv * thisN * targetFrequency;
		CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: Adding solution: HSDIV=" << hsdiv << ", N1=" << thisN << ", fdco_new=" << fdco_new;
		parameter_values.push_back(std::make_pair(thisN, fdco_new));
	}

	auto counter = -1;
	auto newHighSpeedDivider = 0;
	auto newOutputDivider = 0;
	auto newRFREQ = 0.0;

	for (auto values : parameter_values)
	{
		++counter;
		if (values.second > maxFreq) continue;

		newHighSpeedDivider = hsdiv_values[counter];
		newOutputDivider = values.first;
		newRFREQ = values.second / fXTAL;
		break;
	}
	CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: New Program: HSDIV=" << newHighSpeedDivider << ", N1=" << newOutputDivider << ", RFREQ=" << newRFREQ;

	if (EncodeHighSpeedDivider_(newHighSpeedDivider) == -1)
	{
		CFO_TLOG(TLVL_ERROR) << "ERROR: CalculateFrequencyForProgramming: Invalid HSDIV " << newHighSpeedDivider << "!";
		return 0;
	}
	if (newOutputDivider > 128 || newOutputDivider < 0)
	{
		CFO_TLOG(TLVL_ERROR) << "ERROR: CalculateFrequencyForProgramming: Invalid N1 " << newOutputDivider << "!";
		return 0;
	}
	if (newRFREQ <= 0)
	{
		CFO_TLOG(TLVL_ERROR) << "ERROR: CalculateFrequencyForProgramming: Invalid RFREQ " << newRFREQ << "!";
		return 0;
	}

	auto output = (static_cast<uint64_t>(EncodeHighSpeedDivider_(newHighSpeedDivider)) << 45) +
				  (static_cast<uint64_t>(EncodeOutputDivider_(newOutputDivider)) << 38) + EncodeRFREQ_(newRFREQ);
	CFO_TLOG(TLVL_CalculateFreq) << "CalculateFrequencyForProgramming: New Program: " << std::showbase << std::hex << static_cast<unsigned long long>(output);
	return output;
}

/// <summary>
/// Configure the Jitter Attenuator
/// </summary>
void CFOLib::CFO_Registers::ConfigureJitterAttenuator()
{
		// Start configuration preamble
	// set page B
	WriteRegister_(0x68010B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page B registers
	WriteRegister_(0x6824C000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68250000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 5
	WriteRegister_(0x68010500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 5 registers
	WriteRegister_(0x68400100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// End configuration preamble
	//
	// Delay 300 msec
	usleep(300000 /*300ms*/); 

	// Delay is worst case time for device to complete any calibration
	// that is running due to device state change previous to this script
	// being processed.
	//
	// Start configuration registers
	// set page 0
	WriteRegister_(0x68010000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 0 registers
	WriteRegister_(0x68060000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68070000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68080000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680B6800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68160200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6817DC00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68180000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6819DD00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681ADF00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682B0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682C0F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682D5500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682E3700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68303700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68310000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68323700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68330000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68343700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68350000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68363700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68370000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68383700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68390000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683A3700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683C3700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683FFF00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68400400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68410E00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68420E00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68430E00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68440E00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68450C00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68463200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68473200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68483200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68493200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684A3200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684B3200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684C3200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684D3200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684E5500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684F5500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68500F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68510300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68520300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68530300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68540300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68550300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68560300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68570300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68580300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68595500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685AAA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685BAA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685C0A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685D0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685EAA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685FAA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68600A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68610100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6862AA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6863AA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68640A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68650100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6866AA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6867AA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68680A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68690100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68920200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6893A000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68950000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68968000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68986000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689A0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689B6000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689D0800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689E4000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A02000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A20000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A98A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68AA6100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68AB0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68AC0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68E52100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68EA0A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68EB6000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68EC0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68ED0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 1
	WriteRegister_(0x68010100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 1 registers
	WriteRegister_(0x68020100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68120600, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68130900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68143B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68152800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68170600, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68180900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68193B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681A2800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683F1000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68400000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68414000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6842FF00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 2
	WriteRegister_(0x68010200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 2 registers
	WriteRegister_(0x68060000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68086400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68090000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680E0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68100000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68110000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68126400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68130000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68140000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68150000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68160000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68170000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68180100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68190000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681C6400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68200000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68210000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68220100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68230000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68240000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68250000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68266400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68270000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68280000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68290000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682C0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68310B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68320B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68330B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68340B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68350000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68360000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68370000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68388000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6839D400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683EC000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68500000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68510000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68520000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68530000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68540000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68550000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x686B5200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x686C6500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x686D7600, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x686E3100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x686F2000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68702000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68712000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68722000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68900000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68910000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6894B000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68960200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68970200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68990200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689DFA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689E0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A9CC00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68AA0400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68AB0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68B7FF00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 3
	WriteRegister_(0x68010300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 3 registers
	WriteRegister_(0x68020000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68030000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68040000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68050000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68061100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68070000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68080000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68090000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680B8000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68100000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68110000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68120000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68130000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68140000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68150000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68160000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68170000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68380000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68391F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68400000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68410000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68420000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68430000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68440000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68450000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68460000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68590000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 4
	WriteRegister_(0x68010400, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 4 registers
	WriteRegister_(0x68870100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 5
	WriteRegister_(0x68010500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 5 registers
	WriteRegister_(0x68081000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68091F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680A0C00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680B0B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680C3F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680D3F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680E1300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680F2700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68100900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68110800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68123F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68133F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68150000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68160000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68170000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68180000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x6819A800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681A0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681F8000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68212B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682B0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682C8700, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682D0300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682E1900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682F1900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68310000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68324200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68330300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68340000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68350000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68360000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68370000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68380000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68390000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683A0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683B0300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683D1100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683E0600, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68890D00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x688A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689BFA00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689D1000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689E2100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x689F0C00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A00B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A13F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A23F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68A60300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 8
	WriteRegister_(0x68010800, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 8 registers
	WriteRegister_(0x68023500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68030500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68040000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68050000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68060000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68070000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68080000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68090000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x680F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68100000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68110000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68120000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68130000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68140000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68150000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68160000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68170000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68180000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68190000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68200000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68210000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68220000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68230000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68240000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68250000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68260000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68270000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68280000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68290000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x682F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68300000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68310000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68320000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68330000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68340000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68350000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68360000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68370000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68380000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68390000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x683F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68400000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68410000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68420000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68430000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68440000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68450000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68460000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68470000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68480000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68490000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68500000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68510000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68520000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68530000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68540000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68550000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68560000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68570000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68580000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68590000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685B0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685C0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685D0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685F0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68600000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68610000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 9
	WriteRegister_(0x68010900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 9 registers
	WriteRegister_(0x680E0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68430100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68490F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684A0F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684E4900, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684F0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x685E0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page A
	WriteRegister_(0x68010A00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page A registers
	WriteRegister_(0x68020000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68030100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68040100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68050100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68140000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x681A0000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page B
	WriteRegister_(0x68010B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page B registers
	WriteRegister_(0x68442F00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68460000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68470000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68480000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x684A0200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68570E00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68580100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// End configuration registers
	//
	// Start configuration postamble
	// set page 5
	WriteRegister_(0x68010500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 5 registers
	WriteRegister_(0x68140100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 0
	WriteRegister_(0x68010000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 0 registers
	WriteRegister_(0x681C0100, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page 5
	WriteRegister_(0x68010500, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page 5 registers
	WriteRegister_(0x68400000, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// set page B
	WriteRegister_(0x68010B00, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	// page B registers
	WriteRegister_(0x6824C300, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 

	WriteRegister_(0x68250200, CFO_Register_SERDESClock_IICBusLow); 
	WriteRegister_(0x00000001, CFO_Register_SERDESClock_IICBusHigh); 
}
