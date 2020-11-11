#include "DTC_Registers.h"

#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <cmath>
#include <iomanip>  // std::setw, std::setfill
#include <sstream>  // Convert uint to hex string

#include "TRACE/tracemf.h"

#define DTC_TLOG(lvl) TLOG(lvl) << "DTC " << device_.getDTCID() << ": "

#define __SHORTFILE__ \
	(strstr(&__FILE__[0], "/srcs/") ? strstr(&__FILE__[0], "/srcs/") + 6 : __FILE__)
#define __COUT__ std::cout << __SHORTFILE__ << " [" << std::dec << __LINE__ << "]\t"
#define __E__ std::endl
#define Q(X) #X
#define QUOTE(X) Q(X)
#define __COUTV__(X) __COUT__ << QUOTE(X) << " = " << X << __E__

DTCLib::DTC_Registers::DTC_Registers(DTC_SimMode mode, int dtc, std::string simFileName, unsigned rocMask, std::string expectedDesignVersion,
									 bool skipInit)
	: device_(), simMode_(mode),usingDetectorEmulator_(false), dmaSize_(64)
{
	auto sim = getenv("DTCLIB_SIM_ENABLE");
	if (sim != nullptr)
	{
		auto simstr = std::string(sim);
		simMode_ = DTC_SimModeConverter::ConvertToSimMode(simstr);
	}
	TLOG(TLVL_INFO) << "Sim Mode is " << DTC_SimModeConverter(simMode_).toString();

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
	TLOG(TLVL_INFO) << "DTC ID is " << dtc;

       SetSimMode(expectedDesignVersion, simMode_, dtc,simFileName, rocMask, skipInit);
}

DTCLib::DTC_Registers::~DTC_Registers()
{
	DisableDetectorEmulator();
	// DisableDetectorEmulatorMode();
	// DisableCFOEmulation();
	// ResetDTC();
	device_.close();
}

DTCLib::DTC_SimMode DTCLib::DTC_Registers::SetSimMode(std::string expectedDesignVersion, DTC_SimMode mode, int dtc, std::string simMemoryFile,
													  unsigned rocMask, bool skipInit)
{
	simMode_ = mode;
	TLOG(TLVL_INFO) << "Initializing device, sim mode is " << DTC_SimModeConverter(simMode_).toString();
	device_.init(simMode_, dtc, simMemoryFile);
	if (expectedDesignVersion != "" && expectedDesignVersion != ReadDesignVersion())
	{
		throw new DTC_WrongVersionException(expectedDesignVersion, ReadDesignVersion());
	}

	if (skipInit) return simMode_;

	TLOG(TLVL_DEBUG) << "Initialize requested, setting device registers acccording to sim mode " << DTC_SimModeConverter(simMode_).toString();
	bool useTiming = simMode_ == DTC_SimMode_Disabled;
	for (auto link : DTC_Links)
	{
		bool linkEnabled = ((rocMask >> (link * 4)) & 0x1) != 0;
		if (!linkEnabled)
		{
			DisableLink(link);
		}
		else
		{
			EnableLink(link, DTC_LinkEnableMode(true, true, useTiming));
		}
		if (!linkEnabled) DisableROCEmulator(link);
		if (!linkEnabled) SetSERDESLoopbackMode(link, DTC_SERDESLoopbackMode_Disabled);
	}

	if (simMode_ != DTC_SimMode_Disabled)
	{
		// Set up hardware simulation mode: Link 0 Tx/Rx Enabled, Loopback Enabled, ROC Emulator Enabled. All other links
		// disabled. for (auto link : DTC_Links)
		// 	{
		// 	  DisableLink(link);
		// 	}
		//	EnableLink(DTC_Link_0, DTC_LinkEnableMode(true, true, false), DTC_ROC_0);
		for (auto link : DTC_Links)
		{
			if (simMode_ == DTC_SimMode_Loopback)
			{
				SetSERDESLoopbackMode(link, DTC_SERDESLoopbackMode_NearPCS);
				//			SetMaxROCNumber(DTC_Link_0, DTC_ROC_0);
			}
			else if (simMode_ == DTC_SimMode_ROCEmulator)
			{
				EnableROCEmulator(link);
				// SetMaxROCNumber(DTC_Link_0, DTC_ROC_0);
			}
		}
		SetInternalSystemClock();
		DisableTiming();
		SetCFOEmulationMode();
	}
	else
	{
		ClearCFOEmulationMode();
	}
	ReadMinDMATransferLength();

	TLOG(TLVL_DEBUG) << "Done setting device registers";
	return simMode_;
}

//
// DTC Register Dumps
//
std::string DTCLib::DTC_Registers::FormattedRegDump(int width)
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

std::string DTCLib::DTC_Registers::LinkCountersRegDump(int width)
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
std::string DTCLib::DTC_Registers::ReadDesignVersion() { return ReadDesignVersionNumber() + "_" + ReadDesignDate(); }

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDesignVersion()
{
	auto form = CreateFormatter(DTC_Register_DesignVersion);
	form.description = "DTC Firmware Design Version";
	form.vals.push_back(ReadDesignVersionNumber());
	return form;
}

std::string DTCLib::DTC_Registers::ReadDesignDate()
{
	auto data = ReadRegister_(DTC_Register_DesignDate);
	std::ostringstream o;
	int yearHex = (data & 0xFF000000) >> 24;
	auto year = ((yearHex & 0xF0) >> 4) * 10 + (yearHex & 0xF);
	int monthHex = (data & 0xFF0000) >> 16;
	auto month = ((monthHex & 0xF0) >> 4) * 10 + (monthHex & 0xF);
	int dayHex = (data & 0xFF00) >> 8;
	auto day = ((dayHex & 0xF0) >> 4) * 10 + (dayHex & 0xF);
	int hour = ((data & 0xF0) >> 4) * 10 + (data & 0xF);
	o << "20" << std::setfill('0') << std::setw(2) << year << "-";
	o << std::setfill('0') << std::setw(2) << month << "-";
	o << std::setfill('0') << std::setw(2) << day << "-";
	o << std::setfill('0') << std::setw(2) << hour;
	// std::cout << o.str() << std::endl;
	return o.str();
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDesignDate()
{
	auto form = CreateFormatter(DTC_Register_DesignDate);
	form.description = "DTC Firmware Design Date";
	form.vals.push_back(ReadDesignDate());
	return form;
}

std::string DTCLib::DTC_Registers::ReadDesignVersionNumber()
{
	auto data = ReadRegister_(DTC_Register_DesignVersion);
	int minor = data & 0xFF;
	int major = (data & 0xFF00) >> 8;
	return "v" + std::to_string(major) + "." + std::to_string(minor);
}

bool DTCLib::DTC_Registers::ReadDDRInterfaceReset()
{
	return !std::bitset<32>(ReadRegister_(DTC_Register_DesignStatus))[1];
}

void DTCLib::DTC_Registers::SetDDRInterfaceReset(bool reset)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DesignStatus);
	data[1] = !reset;
	WriteRegister_(data.to_ulong(), DTC_Register_DesignStatus);
}

void DTCLib::DTC_Registers::ResetDDRInterface()
{
	SetDDRInterfaceReset(true);
	usleep(1000);
	SetDDRInterfaceReset(false);
	while (ReadDDRInterfaceReset() && !ReadDDRAutoCalibrationDone())
	{
		usleep(1000);
	}
}

bool DTCLib::DTC_Registers::ReadDDRAutoCalibrationDone()
{
	return std::bitset<32>(ReadRegister_(DTC_Register_DesignStatus))[0];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDesignStatus()
{
	auto form = CreateFormatter(DTC_Register_DesignStatus);
	form.description = "Control and Status";
	form.vals.push_back(std::string("Reset DDR Interface:       [") + (ReadDDRInterfaceReset() ? "x" : " ") + "]");
	form.vals.push_back(std::string("DDR Auto-Calibration Done: [") + (ReadDDRAutoCalibrationDone() ? "x" : " ") + "]");
	return form;
}

std::string DTCLib::DTC_Registers::ReadVivadoVersionNumber()
{
	auto data = ReadRegister_(DTC_Register_VivadoVersion);
	std::ostringstream o;
	int yearHex = (data & 0xFFFF0000) >> 16;
	auto year = ((yearHex & 0xF000) >> 12) * 1000 + ((yearHex & 0xF00) >> 8) * 100 + ((yearHex & 0xF0) >> 4) * 10 +
				(yearHex & 0xF);
	int versionHex = (data & 0xFFFF);
	auto version = ((versionHex & 0xF000) >> 12) * 1000 + ((versionHex & 0xF00) >> 8) * 100 +
				   ((versionHex & 0xF0) >> 4) * 10 + (versionHex & 0xF);
	o << std::setfill('0') << std::setw(4) << year << "-" << version;
	// std::cout << o.str() << std::endl;
	return o.str();
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatVivadoVersion()
{
	auto form = CreateFormatter(DTC_Register_VivadoVersion);
	form.description = "DTC Firmware Vivado Version";
	form.vals.push_back(ReadVivadoVersionNumber());
	return form;
}

double DTCLib::DTC_Registers::ReadFPGATemperature()
{
	auto val = ReadRegister_(DTC_Register_FPGA_Temperature);

	return ((val * 503.975) / 4096.0) - 273.15;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGATemperature()
{
	auto form = CreateFormatter(DTC_Register_FPGA_Temperature);
	form.description = "FPGA Temperature";
	form.vals.push_back(std::to_string(ReadFPGATemperature()) + " C");
	return form;
}

double DTCLib::DTC_Registers::ReadFPGAVCCINTVoltage()
{
	auto val = ReadRegister_(DTC_Register_FPGA_VCCINT);
	return (val / 4095.0) * 3.0;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGAVCCINT()
{
	auto form = CreateFormatter(DTC_Register_FPGA_VCCINT);
	form.description = "FPGA VCC INT";
	form.vals.push_back(std::to_string(ReadFPGAVCCINTVoltage()) + " V");
	return form;
}

double DTCLib::DTC_Registers::ReadFPGAVCCAUXVoltage()
{
	auto val = ReadRegister_(DTC_Register_FPGA_VCCAUX);
	return (val / 4095.0) * 3.0;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGAVCCAUX()
{
	auto form = CreateFormatter(DTC_Register_FPGA_VCCAUX);
	form.description = "FPGA VCC AUX";
	form.vals.push_back(std::to_string(ReadFPGAVCCAUXVoltage()) + " V");
	return form;
}

double DTCLib::DTC_Registers::ReadFPGAVCCBRAMVoltage()
{
	auto val = ReadRegister_(DTC_Register_FPGA_VCCBRAM);
	return (val / 4095.0) * 3.0;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGAVCCBRAM()
{
	auto form = CreateFormatter(DTC_Register_FPGA_VCCBRAM);
	form.description = "FPGA VCC BRAM";
	form.vals.push_back(std::to_string(ReadFPGAVCCBRAMVoltage()) + " V");
	return form;
}

bool DTCLib::DTC_Registers::ReadFPGADieTemperatureAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[8];
}

void DTCLib::DTC_Registers::ResetFPGADieTemperatureAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[8] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

bool DTCLib::DTC_Registers::ReadFPGAAlarms()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[7];
}

void DTCLib::DTC_Registers::ResetFPGAAlarms()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[7] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

bool DTCLib::DTC_Registers::ReadVCCBRAMAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[3];
}

void DTCLib::DTC_Registers::ResetVCCBRAMAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[3] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

bool DTCLib::DTC_Registers::ReadVCCAUXAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[2];
}

void DTCLib::DTC_Registers::ResetVCCAUXAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[2] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

bool DTCLib::DTC_Registers::ReadVCCINTAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[1];
}

void DTCLib::DTC_Registers::ResetVCCINTAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

bool DTCLib::DTC_Registers::ReadFPGAUserTemperatureAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	return data[0];
}

void DTCLib::DTC_Registers::ResetFPGAUserTemperatureAlarm()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FPGA_MonitorAlarm);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FPGA_MonitorAlarm);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGAAlarms()
{
	auto form = CreateFormatter(DTC_Register_FPGA_MonitorAlarm);
	form.description = "FPGA Monitor Alarm";
	form.vals.push_back(std::string("FPGA Die Temperature Alarm:  [") + (ReadFPGADieTemperatureAlarm() ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA Alarms OR:              [") + (ReadFPGAAlarms() ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC BRAM Alarm:              [") + (ReadVCCBRAMAlarm() ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC AUX Alarm:               [") + (ReadVCCAUXAlarm() ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC INT Alarm:               [") + (ReadVCCINTAlarm() ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA User Temperature Alarm: [") + (ReadFPGAUserTemperatureAlarm() ? "x" : " ") + "]");

	return form;
}

// DTC Control Register
void DTCLib::DTC_Registers::ResetDTC()
{
	DTC_TLOG(15) << "ResetDTC start";
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[31] = 1;  // DTC Reset bit
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadResetDTC()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_DTCControl);
	return dataSet[31];
}

void DTCLib::DTC_Registers::EnableCFOEmulation()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[30] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableCFOEmulation()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[30] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadCFOEmulation()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_DTCControl);
	return dataSet[30];
}

void DTCLib::DTC_Registers::EnableCFOLoopback()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[28] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableCFOLoopback()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[28] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadCFOLoopback()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_DTCControl);
	return dataSet[28];
}

void DTCLib::DTC_Registers::ResetDDRWriteAddress()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[27] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
	usleep(1000);
	data = ReadRegister_(DTC_Register_DTCControl);
	data[27] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadResetDDRWriteAddress()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[27];
}

void DTCLib::DTC_Registers::ResetDDRReadAddress()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[26] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
	usleep(1000);
	data = ReadRegister_(DTC_Register_DTCControl);
	data[26] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadResetDDRReadAddress()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[26];
}

void DTCLib::DTC_Registers::ResetDDR()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[25] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
	usleep(1000);
	data = ReadRegister_(DTC_Register_DTCControl);
	data[25] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadResetDDR()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[25];
}

void DTCLib::DTC_Registers::EnableCFOEmulatorDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[24] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableCFOEmulatorDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[24] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadCFOEmulatorDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[24];
}

void DTCLib::DTC_Registers::EnableAutogenDRP()
{
	DTC_TLOG(15) << "EnableAutogenDRP start";
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[23] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableAutogenDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[23] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadAutogenDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[23];
}

void DTCLib::DTC_Registers::EnableSoftwareDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[22] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableSoftwareDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[22] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadSoftwareDRP()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[22];
}

void DTCLib::DTC_Registers::EnableLED6()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[16] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableLED6()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[16] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadLED6State()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[16];
}

void DTCLib::DTC_Registers::SetCFOEmulationMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[15] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearCFOEmulationMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[15] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadCFOEmulationMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[15];
}

void DTCLib::DTC_Registers::SetDataFilterEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[13] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearDataFilterEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[13] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadDataFilterEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[13];
}

void DTCLib::DTC_Registers::SetDRPPrefetchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[12] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearDRPPrefetchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[12] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadDRPPrefetchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[12];
}

void DTCLib::DTC_Registers::SetSequenceNumberEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[10] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearSequenceNumberEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[10] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadSequenceNumberEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[10];
}

void DTCLib::DTC_Registers::SetPunchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[9] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearPunchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[9] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadPunchEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[9];
}

void DTCLib::DTC_Registers::ResetSERDES()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[8] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
	while (ReadResetSERDES())
	{
		usleep(1000);
	}
}

bool DTCLib::DTC_Registers::ReadResetSERDES()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[8];
}

void DTCLib::DTC_Registers::SetRxPacketErrorFeedbackEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[6] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearRxPacketErrorFeedbackEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[6] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadRxPacketErrorFeedbackEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[6];
}

void DTCLib::DTC_Registers::SetCommaToleranceEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[5] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::ClearCommaToleranceEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[5] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadCommaToleranceEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[5];
}

void DTCLib::DTC_Registers::SetExternalFanoutClockInput()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[4] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::SetInternalFanoutClockInput()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[4] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadFanoutClockInput()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[4];
}

void DTCLib::DTC_Registers::EnableDCSReception()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[2] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}
void DTCLib::DTC_Registers::DisableDCSReception()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[2] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}
bool DTCLib::DTC_Registers::ReadDCSReception()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[2];
}

void DTCLib::DTC_Registers::EnableTiming()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

void DTCLib::DTC_Registers::DisableTiming()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	data[0] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DTCControl);
}

bool DTCLib::DTC_Registers::ReadTimingEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DTCControl);
	return data[0];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDTCControl()
{
	auto form = CreateFormatter(DTC_Register_DTCControl);
	form.description = "DTC Control";
	form.vals.push_back(std::string("Reset:                           [") + (ReadResetDTC() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Emulation Enable:            [") + (ReadCFOEmulation() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Link Output Control:         [") + (ReadCFOLoopback() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Reset DDR Write Address:         [") + (ReadResetDDRWriteAddress() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Reset DDR Read Address:          [") + (ReadResetDDRReadAddress() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Reset DDR:                       [") + (ReadResetDDR() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Emulator DRP Enable:         [") + (ReadCFOEmulatorDRP() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Autogenerate DRP:            [") + (ReadAutogenDRP() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Software DRP Enable:             [") + (ReadSoftwareDRP() ? "x" : " ") + "]");
	form.vals.push_back(std::string("LED 6:                           [") + (ReadLED6State() ? "x" : " ") + "]");
	form.vals.push_back(std::string("CFO Emulation Mode:              [") + (ReadCFOEmulationMode() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Filter Enable:              [") + (ReadDataFilterEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("DRP Prefetch Enable:             [") + (ReadDRPPrefetchEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Sequence Number Enable:          [") + (ReadSequenceNumberEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Punch Enable:                    [") + (ReadPunchEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SERDES Global Reset:             [") + (ReadResetSERDES() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error Feedback Enable: [") + (ReadRxPacketErrorFeedbackEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Comma Tolerance Enable:          [") + (ReadCommaToleranceEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Fanout Input Select:             [") + (ReadFanoutClockInput() ? "E" : "I") + "]");
	form.vals.push_back(std::string("DCS Enable:                      [") + (ReadDCSReception() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Timing Enable:                   [") + (ReadTimingEnable() ? "x" : " ") + "]");
	return form;
}

// DMA Transfer Length Register
void DTCLib::DTC_Registers::SetTriggerDMATransferLength(uint16_t length)
{
	auto data = ReadRegister_(DTC_Register_DMATransferLength);
	data = (data & 0x0000FFFF) + (length << 16);
	WriteRegister_(data, DTC_Register_DMATransferLength);
}

uint16_t DTCLib::DTC_Registers::ReadTriggerDMATransferLength()
{
	auto data = ReadRegister_(DTC_Register_DMATransferLength);
	data >>= 16;
	return static_cast<uint16_t>(data);
}

void DTCLib::DTC_Registers::SetMinDMATransferLength(uint16_t length)
{
	auto data = ReadRegister_(DTC_Register_DMATransferLength);
	data = (data & 0xFFFF0000) + length;
	WriteRegister_(data, DTC_Register_DMATransferLength);
}

uint16_t DTCLib::DTC_Registers::ReadMinDMATransferLength()
{
	auto data = ReadRegister_(DTC_Register_DMATransferLength);
	data = data & 0x0000FFFF;
	dmaSize_ = static_cast<uint16_t>(data);
	return dmaSize_;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDMATransferLength()
{
	auto form = CreateFormatter(DTC_Register_DMATransferLength);
	form.description = "DMA Transfer Length";
	std::stringstream o;
	o << "Trigger Length: 0x" << std::hex << ReadTriggerDMATransferLength();
	form.vals.push_back(o.str());
	std::stringstream p;
	p << "Minimum Length: 0x" << std::hex << ReadMinDMATransferLength();
	form.vals.push_back(p.str());
	return form;
}

// SERDES Loopback Enable Register
void DTCLib::DTC_Registers::SetSERDESLoopbackMode(const DTC_Link_ID& link, const DTC_SERDESLoopbackMode& mode)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDESLoopbackEnable);
	std::bitset<3> modeSet = mode;
	data[3 * link] = modeSet[0];
	data[3 * link + 1] = modeSet[1];
	data[3 * link + 2] = modeSet[2];
	WriteRegister_(data.to_ulong(), DTC_Register_SERDESLoopbackEnable);
}

DTCLib::DTC_SERDESLoopbackMode DTCLib::DTC_Registers::ReadSERDESLoopback(const DTC_Link_ID& link)
{
	std::bitset<3> dataSet = ReadRegister_(DTC_Register_SERDESLoopbackEnable) >> 3 * link;
	return static_cast<DTC_SERDESLoopbackMode>(dataSet.to_ulong());
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESLoopbackEnable()
{
	auto form = CreateFormatter(DTC_Register_SERDESLoopbackEnable);
	form.description = "SERDES Loopback Enable";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": " +
							DTC_SERDESLoopbackModeConverter(ReadSERDESLoopback(r)).toString());
	}
	form.vals.push_back(std::string("CFO:    ") +
						DTC_SERDESLoopbackModeConverter(ReadSERDESLoopback(DTC_Link_CFO)).toString());
	form.vals.push_back(std::string("EVB:    ") +
						DTC_SERDESLoopbackModeConverter(ReadSERDESLoopback(DTC_Link_EVB)).toString());
	return form;
}

// Clock Status Register
bool DTCLib::DTC_Registers::ReadSERDESOscillatorIICError()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ClockOscillatorStatus);
	return dataSet[2];
}

bool DTCLib::DTC_Registers::ReadDDROscillatorIICError()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ClockOscillatorStatus);
	return dataSet[18];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatClockOscillatorStatus()
{
	auto form = CreateFormatter(DTC_Register_ClockOscillatorStatus);
	form.description = "Clock Oscillator Status";
	form.vals.push_back(std::string("SERDES IIC Error:     [") + (ReadSERDESOscillatorIICError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("DDR IIC Error:        [") + (ReadDDROscillatorIICError() ? "x" : " ") + "]");
	return form;
}

// ROC Emulation Enable Register
void DTCLib::DTC_Registers::EnableROCEmulator(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ROCEmulationEnable);
	dataSet[link] = 1;
	WriteRegister_(dataSet.to_ulong(), DTC_Register_ROCEmulationEnable);
}

void DTCLib::DTC_Registers::DisableROCEmulator(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ROCEmulationEnable);
	dataSet[link] = 0;
	WriteRegister_(dataSet.to_ulong(), DTC_Register_ROCEmulationEnable);
}

bool DTCLib::DTC_Registers::ReadROCEmulator(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ROCEmulationEnable);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatROCEmulationEnable()
{
	auto form = CreateFormatter(DTC_Register_ROCEmulationEnable);
	form.description = "ROC Emulator Enable";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadROCEmulator(r) ? "x" : " ") + "]");
	}
	return form;
}

// Link Enable Register
void DTCLib::DTC_Registers::EnableLink(const DTC_Link_ID& link, const DTC_LinkEnableMode& mode)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_LinkEnable);
	data[link] = mode.TransmitEnable;
	data[link + 8] = mode.ReceiveEnable;
	data[link + 16] = mode.TimingEnable;
	WriteRegister_(data.to_ulong(), DTC_Register_LinkEnable);
}

void DTCLib::DTC_Registers::DisableLink(const DTC_Link_ID& link, const DTC_LinkEnableMode& mode)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_LinkEnable);
	data[link] = data[link] && !mode.TransmitEnable;
	data[link + 8] = data[link + 8] && !mode.ReceiveEnable;
	data[link + 16] = data[link + 16] && !mode.TimingEnable;
	WriteRegister_(data.to_ulong(), DTC_Register_LinkEnable);
}

DTCLib::DTC_LinkEnableMode DTCLib::DTC_Registers::ReadLinkEnabled(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_LinkEnable);
	return DTC_LinkEnableMode(dataSet[link], dataSet[link + 8], dataSet[link + 16]);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRingEnable()
{
	auto form = CreateFormatter(DTC_Register_LinkEnable);
	form.description = "Link Enable";
	form.vals.push_back("       ([TX, RX, Timing])");
	for (auto r : DTC_Links)
	{
		auto re = ReadLinkEnabled(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.TransmitEnable ? "x" : " ") + "," +
							(re.ReceiveEnable ? "x" : " ") + "," + (re.TimingEnable ? "x" : " ") + "]");
	}
	{
		auto ce = ReadLinkEnabled(DTC_Link_CFO);
		form.vals.push_back(std::string("CFO:    [") + "TX:[" + (ce.TransmitEnable ? "x" : " ") + "], " + "RX:[" +
							(ce.ReceiveEnable ? "x" : " ") + "]]");
	}
	{
		auto ee = ReadLinkEnabled(DTC_Link_EVB);
		form.vals.push_back(std::string("EVB:    [") + "TX:[" + (ee.TransmitEnable ? "x" : " ") + "], " + "RX:[" +
							(ee.ReceiveEnable ? "x" : " ") + "]]");
	}
	return form;
}

// SERDES Reset Register
void DTCLib::DTC_Registers::ResetSERDESTX(const DTC_Link_ID& link, int interval)
{
	auto resetDone = false;
	while (!resetDone)
	{
		DTC_TLOG(4) << "Entering SERDES Reset Loop for Link " << link;
		std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link + 24] = 1;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link + 24] = 0;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		resetDone = ReadResetTXSERDESDone(link);
		DTC_TLOG(4) << "End of SERDES Reset loop, done=" << std::boolalpha << resetDone;
	}
}

bool DTCLib::DTC_Registers::ReadResetSERDESTX(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_Reset);
	return dataSet[link + 24];
}

void DTCLib::DTC_Registers::ResetSERDESRX(const DTC_Link_ID& link, int interval)
{
	auto resetDone = false;
	while (!resetDone)
	{
		DTC_TLOG(4) << "Entering SERDES Reset Loop for Link " << link;
		std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link + 16] = 1;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link + 16] = 0;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		resetDone = ReadResetRXSERDESDone(link);
		DTC_TLOG(4) << "End of SERDES Reset loop, done=" << std::boolalpha << resetDone;
	}
}

bool DTCLib::DTC_Registers::ReadResetSERDESRX(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_Reset);
	return dataSet[link + 16];
}

void DTCLib::DTC_Registers::ResetSERDES(const DTC_Link_ID& link, int interval)
{
	auto resetDone = false;
	while (!resetDone)
	{
		DTC_TLOG(4) << "Entering SERDES Reset Loop for Link " << link;
		std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link] = 1;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		data = ReadRegister_(DTC_Register_SERDES_Reset);
		data[link] = 0;
		WriteRegister_(data.to_ulong(), DTC_Register_SERDES_Reset);

		usleep(interval);

		resetDone = ReadResetRXSERDESDone(link);
		resetDone = resetDone && ReadResetTXSERDESDone(link);
		DTC_TLOG(4) << "End of SERDES Reset loop, done=" << std::boolalpha << resetDone;
	}
}

bool DTCLib::DTC_Registers::ReadResetSERDES(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_Reset);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESReset()
{
	auto form = CreateFormatter(DTC_Register_SERDES_Reset);
	form.description = "SERDES Reset";
	form.vals.push_back("       ([TX,RX,Link])");
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadResetSERDESTX(r) ? "x" : " ") + (ReadResetSERDESRX(r) ? "x" : " ") + (ReadResetSERDES(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadResetSERDESTX(DTC_Link_CFO) ? "x" : " ") + (ReadResetSERDESRX(DTC_Link_CFO) ? "x" : " ") + (ReadResetSERDES(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadResetSERDESTX(DTC_Link_EVB) ? "x" : " ") + (ReadResetSERDESRX(DTC_Link_EVB) ? "x" : " ") + (ReadResetSERDES(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// SERDES RX Disparity Error Register
DTCLib::DTC_SERDESRXDisparityError DTCLib::DTC_Registers::ReadSERDESRXDisparityError(const DTC_Link_ID& link)
{
	return DTC_SERDESRXDisparityError(ReadRegister_(DTC_Register_SERDES_RXDisparityError), link);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESRXDisparityError()
{
	auto form = CreateFormatter(DTC_Register_SERDES_RXDisparityError);
	form.description = "SERDES RX Disparity Error";
	form.vals.push_back("       ([H,L])");
	for (auto r : DTC_Links)
	{
		auto re = ReadSERDESRXDisparityError(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + to_string(re.GetData()[1]) + "," +
							to_string(re.GetData()[0]) + "]");
	}
	auto ce = ReadSERDESRXDisparityError(DTC_Link_CFO);
	form.vals.push_back(std::string("CFO:    [") + to_string(ce.GetData()[1]) + "," + to_string(ce.GetData()[0]) + "]");
	auto ee = ReadSERDESRXDisparityError(DTC_Link_EVB);
	form.vals.push_back(std::string("EVB:    [") + to_string(ee.GetData()[1]) + "," + to_string(ee.GetData()[0]) + "]");
	return form;
}

// SERDES RX Character Not In Table Error Register
DTCLib::DTC_CharacterNotInTableError DTCLib::DTC_Registers::ReadSERDESRXCharacterNotInTableError(
	const DTC_Link_ID& link)
{
	return DTC_CharacterNotInTableError(ReadRegister_(DTC_Register_SERDES_RXCharacterNotInTableError), link);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESRXCharacterNotInTableError()
{
	auto form = CreateFormatter(DTC_Register_SERDES_RXCharacterNotInTableError);
	form.description = "SERDES RX CNIT Error";
	form.vals.push_back("       ([H,L])");
	for (auto r : DTC_Links)
	{
		auto re = ReadSERDESRXCharacterNotInTableError(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + to_string(re.GetData()[1]) + "," +
							to_string(re.GetData()[0]) + "]");
	}
	auto ce = ReadSERDESRXCharacterNotInTableError(DTC_Link_CFO);
	form.vals.push_back(std::string("CFO:    [") + to_string(ce.GetData()[1]) + "," + to_string(ce.GetData()[0]) + "]");
	auto ee = ReadSERDESRXCharacterNotInTableError(DTC_Link_EVB);
	form.vals.push_back(std::string("EVB:    [") + to_string(ee.GetData()[1]) + "," + to_string(ee.GetData()[0]) + "]");
	return form;
}

// SERDES Unlock Error Register
bool DTCLib::DTC_Registers::ReadSERDESUnlockError(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_UnlockError);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESUnlockError()
{
	auto form = CreateFormatter(DTC_Register_SERDES_UnlockError);
	form.description = "SERDES Unlock Error";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESUnlockError(r) ? "x" : " ") +
							"]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadSERDESUnlockError(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadSERDESUnlockError(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// SERDES PLL Locked Register
bool DTCLib::DTC_Registers::ReadSERDESPLLLocked(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_PLLLocked);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESPLLLocked()
{
	auto form = CreateFormatter(DTC_Register_SERDES_PLLLocked);
	form.description = "SERDES PLL Locked";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESPLLLocked(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadSERDESPLLLocked(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadSERDESPLLLocked(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

void DTCLib::DTC_Registers::EnableSERDESPLL(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_PLLPowerDown);
	data[link] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_SERDES_PLLPowerDown);
}

void DTCLib::DTC_Registers::DisableSERDESPLL(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_PLLPowerDown);
	data[link] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_SERDES_PLLPowerDown);
}

bool DTCLib::DTC_Registers::ReadSERDESPLLPowerDown(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDES_PLLPowerDown);
	return data[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESPLLPowerDown()
{
	auto form = CreateFormatter(DTC_Register_SERDES_PLLPowerDown);
	form.description = "SERDES PLL Power Down";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadSERDESPLLPowerDown(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadSERDESPLLPowerDown(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadSERDESPLLPowerDown(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// SERDES RX Status Register
DTCLib::DTC_RXStatus DTCLib::DTC_Registers::ReadSERDESRXStatus(const DTC_Link_ID& link)
{
	std::bitset<3> dataSet = ReadRegister_(DTC_Register_SERDES_RXStatus) >> 3 * link;
	return static_cast<DTC_RXStatus>(dataSet.to_ulong());
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESRXStatus()
{
	auto form = CreateFormatter(DTC_Register_SERDES_RXStatus);
	form.description = "SERDES RX Status";
	for (auto r : DTC_Links)
	{
		auto re = ReadSERDESRXStatus(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": " + DTC_RXStatusConverter(re).toString());
	}
	auto ce = ReadSERDESRXStatus(DTC_Link_CFO);
	form.vals.push_back(std::string("CFO:    ") + DTC_RXStatusConverter(ce).toString());
	auto ee = ReadSERDESRXStatus(DTC_Link_EVB);
	form.vals.push_back(std::string("EVB:    ") + DTC_RXStatusConverter(ee).toString());

	return form;
}

// SERDES Reset Done Register
bool DTCLib::DTC_Registers::ReadResetRXFSMSERDESDone(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_ResetDone);
	return dataSet[link + 24];
}
bool DTCLib::DTC_Registers::ReadResetRXSERDESDone(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_ResetDone);
	return dataSet[link + 16];
}
bool DTCLib::DTC_Registers::ReadResetTXFSMSERDESDone(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_ResetDone);
	return dataSet[link + 8];
}
bool DTCLib::DTC_Registers::ReadResetTXSERDESDone(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_ResetDone);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESResetDone()
{
	auto form = CreateFormatter(DTC_Register_SERDES_ResetDone);
	form.description = "SERDES Reset Done";
	form.vals.push_back("       ([RX FSM, RX, TX FSM, TX])");
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadResetRXFSMSERDESDone(r) ? "x" : " ") + (ReadResetRXSERDESDone(r) ? "x" : " ") + (ReadResetTXFSMSERDESDone(r) ? "x" : " ") + (ReadResetTXSERDESDone(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadResetRXFSMSERDESDone(DTC_Link_CFO) ? "x" : " ") + (ReadResetRXSERDESDone(DTC_Link_CFO) ? "x" : " ") + (ReadResetTXFSMSERDESDone(DTC_Link_CFO) ? "x" : " ") + (ReadResetTXSERDESDone(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadResetRXFSMSERDESDone(DTC_Link_EVB) ? "x" : " ") + (ReadResetRXSERDESDone(DTC_Link_EVB) ? "x" : " ") + (ReadResetTXFSMSERDESDone(DTC_Link_EVB) ? "x" : " ") + (ReadResetTXSERDESDone(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// SFP / SERDES Status Register

bool DTCLib::DTC_Registers::ReadSERDESRXCDRLock(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_SERDES_RXCDRLockStatus);
	return dataSet[link];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRXCDRLockStatus()
{
	auto form = CreateFormatter(DTC_Register_SERDES_RXCDRLockStatus);
	form.description = "RX CDR Lock Status";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) +
							" CDR Lock: " + (ReadSERDESRXCDRLock(r) ? "x" : " "));
	}
	form.vals.push_back(std::string("CFO CDR Lock: ") + +(ReadSERDESRXCDRLock(DTC_Link_CFO) ? "x" : " "));
	form.vals.push_back(std::string("EVB CDR Lock: ") + (ReadSERDESRXCDRLock(DTC_Link_EVB) ? "x" : " "));
	return form;
}

// DMA Timeout Preset Register
void DTCLib::DTC_Registers::SetDMATimeoutPreset(uint32_t preset)
{
	WriteRegister_(preset, DTC_Register_DMATimeoutPreset);
}

uint32_t DTCLib::DTC_Registers::ReadDMATimeoutPreset() { return ReadRegister_(DTC_Register_DMATimeoutPreset); }

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDMATimeoutPreset()
{
	auto form = CreateFormatter(DTC_Register_DMATimeoutPreset);
	form.description = "DMA Timeout";
	std::stringstream o;
	o << "0x" << std::hex << ReadDMATimeoutPreset();
	form.vals.push_back(o.str());
	return form;
}

// ROC Timeout Preset Register
uint32_t DTCLib::DTC_Registers::ReadROCTimeoutPreset() { return ReadRegister_(DTC_Register_ROCReplyTimeout); }

void DTCLib::DTC_Registers::SetROCTimeoutPreset(uint32_t preset)
{
	WriteRegister_(preset, DTC_Register_ROCReplyTimeout);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatROCReplyTimeout()
{
	auto form = CreateFormatter(DTC_Register_ROCReplyTimeout);
	form.description = "ROC Reply Timeout";
	std::stringstream o;
	o << "0x" << std::hex << ReadROCTimeoutPreset();
	form.vals.push_back(o.str());
	return form;
}

// ROC Timeout Error Register
bool DTCLib::DTC_Registers::ReadROCTimeoutError(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_ROCReplyTimeoutError);
	return data[static_cast<int>(link)];
}

void DTCLib::DTC_Registers::ClearROCTimeoutError(const DTC_Link_ID& link)
{
	std::bitset<32> data = 0x0;
	data[link] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_ROCReplyTimeoutError);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatROCReplyTimeoutError()
{
	auto form = CreateFormatter(DTC_Register_ROCReplyTimeoutError);
	form.description = "ROC Reply Timeout Error";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadROCTimeoutError(r) ? "x" : " ") + "]");
	}
	return form;
}

// Link Packet Length Register
void DTCLib::DTC_Registers::SetPacketSize(uint16_t packetSize)
{
	WriteRegister_(0x00000000 + packetSize, DTC_Register_LinkPacketLength);
}

uint16_t DTCLib::DTC_Registers::ReadPacketSize()
{
	return static_cast<uint16_t>(ReadRegister_(DTC_Register_LinkPacketLength));
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRingPacketLength()
{
	auto form = CreateFormatter(DTC_Register_LinkPacketLength);
	form.description = "DMA Link Packet Length";
	std::stringstream o;
	o << "0x" << std::hex << ReadPacketSize();
	form.vals.push_back(o.str());
	return form;
}

// EVB Network Partition ID / EVB Network Local MAC Index Register
void DTCLib::DTC_Registers::SetEVBMode(uint8_t mode)
{
	auto regVal = ReadRegister_(DTC_Register_EVBPartitionID) & 0xFF00FFFF;
	regVal += mode << 16;
	WriteRegister_(regVal, DTC_Register_EVBPartitionID);
}

uint8_t DTCLib::DTC_Registers::ReadEVBMode()
{
	auto regVal = ReadRegister_(DTC_Register_EVBPartitionID) & 0xFF0000;
	return static_cast<uint8_t>(regVal >> 16);
}

void DTCLib::DTC_Registers::SetEVBLocalParitionID(uint8_t id)
{
	auto regVal = ReadRegister_(DTC_Register_EVBPartitionID) & 0xFFFFFCFF;
	regVal += (id & 0x3) << 8;
	WriteRegister_(regVal, DTC_Register_EVBPartitionID);
}

uint8_t DTCLib::DTC_Registers::ReadEVBLocalParitionID()
{
	auto regVal = ReadRegister_(DTC_Register_EVBPartitionID) & 0xFF0000;
	return static_cast<uint8_t>((regVal >> 8) & 0x3);
}

void DTCLib::DTC_Registers::SetEVBLocalMACAddress(uint8_t macByte)
{
	auto regVal = ReadRegister_(DTC_Register_EVBPartitionID) & 0xFFFFFFC0;
	regVal += (macByte & 0x3F);
	WriteRegister_(regVal, DTC_Register_EVBPartitionID);
}

uint8_t DTCLib::DTC_Registers::ReadEVBLocalMACAddress() { return ReadRegister_(DTC_Register_EVBPartitionID) & 0x3F; }

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatEVBLocalParitionIDMACIndex()
{
	auto form = CreateFormatter(DTC_Register_EVBPartitionID);
	form.description = "EVB Local Partition ID / MAC Index";
	std::ostringstream o;
	o << "EVB Mode: 0x" << std::hex << static_cast<int>(ReadEVBMode());
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "EVB Local Parition ID: 0x" << std::hex << static_cast<int>(ReadEVBLocalParitionID());
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "EVB MAC Index:         0x" << std::hex << static_cast<int>(ReadEVBLocalMACAddress());
	form.vals.push_back(o.str());
	return form;
}

// EVB Number of Destination Nodes Register

void DTCLib::DTC_Registers::SetEVBStartNode(uint8_t node)
{
	auto regVal = ReadRegister_(DTC_Register_EVBDestCount) & 0xFFFFC0FF;
	regVal += (node & 0x3F) << 8;
	WriteRegister_(regVal, DTC_Register_EVBDestCount);
}

uint8_t DTCLib::DTC_Registers::ReadEVBStartNode()
{
	return static_cast<uint8_t>((ReadRegister_(DTC_Register_EVBDestCount) & 0x3F00) >> 8);
}

void DTCLib::DTC_Registers::SetEVBNumberOfDestinationNodes(uint8_t number)
{
	auto regVal = ReadRegister_(DTC_Register_EVBDestCount) & 0xFFFFFFC0;
	regVal += (number & 0x3F);
	WriteRegister_(regVal, DTC_Register_EVBDestCount);
}

uint8_t DTCLib::DTC_Registers::ReadEVBNumberOfDestinationNodes()
{
	return static_cast<uint8_t>(ReadRegister_(DTC_Register_EVBDestCount) & 0x3F);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatEVBNumberOfDestinationNodes()
{
	auto form = CreateFormatter(DTC_Register_EVBDestCount);
	form.description = "EVB Number of Destination Nodes / Start Node";
	std::stringstream o;
	o << "EVB Start Node: " << std::dec << static_cast<int>(ReadEVBStartNode());
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "EVB Number of Destination Nodes: " << std::dec << static_cast<int>(ReadEVBNumberOfDestinationNodes());
	form.vals.push_back(o.str());
	return form;
}

// SEREDES Oscillator Registers
uint32_t DTCLib::DTC_Registers::ReadSERDESOscillatorReferenceFrequency(DTCLib::DTC_IICSERDESBusAddress device)
{
	switch (device)
	{
		case DTC_IICSERDESBusAddress_CFO:
			return ReadRegister_(DTC_Register_SERDESTimingCardOscillatorFrequency);
		case DTC_IICSERDESBusAddress_EVB:
			return ReadRegister_(DTC_Register_SERDESMainBoardOscillatorFrequency);
		default:
			return 0;
	}
	return 0;
}
void DTCLib::DTC_Registers::SetSERDESOscillatorReferenceFrequency(DTCLib::DTC_IICSERDESBusAddress device,
																  uint32_t freq)
{
	switch (device)
	{
		case DTC_IICSERDESBusAddress_CFO:
			return WriteRegister_(freq, DTC_Register_SERDESTimingCardOscillatorFrequency);
		case DTC_IICSERDESBusAddress_EVB:
			return WriteRegister_(freq, DTC_Register_SERDESMainBoardOscillatorFrequency);
		default:
			return;
	}
	return;
}

bool DTCLib::DTC_Registers::ReadSERDESOscillatorIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(DTC_Register_SERDESOscillatorIICBusControl));
	return dataSet[31];
}

void DTCLib::DTC_Registers::ResetSERDESOscillatorIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), DTC_Register_SERDESOscillatorIICBusControl);
	while (ReadSERDESOscillatorIICInterfaceReset())
	{
		usleep(1000);
	}
}

void DTCLib::DTC_Registers::WriteSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, DTC_Register_SERDESOscillatorIICBusLow);
	WriteRegister_(0x1, DTC_Register_SERDESOscillatorIICBusHigh);
	while (ReadRegister_(DTC_Register_SERDESOscillatorIICBusHigh) == 0x1)
	{
		usleep(1000);
	}
}

uint8_t DTCLib::DTC_Registers::ReadSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, DTC_Register_SERDESOscillatorIICBusLow);
	WriteRegister_(0x2, DTC_Register_SERDESOscillatorIICBusHigh);
	while (ReadRegister_(DTC_Register_SERDESOscillatorIICBusHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(DTC_Register_SERDESOscillatorIICBusLow);
	return static_cast<uint8_t>(data);
}

DTCLib::DTC_SerdesClockSpeed DTCLib::DTC_Registers::ReadSERDESOscillatorClock()
{
	auto freq = ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_EVB);

	// Clocks should be accurate to 30 ppm
	if (freq > 156250000 - 4687.5 && freq < 156250000 + 4687.5) return DTC_SerdesClockSpeed_3125Gbps;
	if (freq > 125000000 - 3750 && freq < 125000000 + 3750) return DTC_SerdesClockSpeed_25Gbps;
	return DTC_SerdesClockSpeed_Unknown;
}
void DTCLib::DTC_Registers::SetSERDESOscillatorClock(DTC_SerdesClockSpeed speed)
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
		default:
			targetFreq = 0.0;
			break;
	}
	if (SetNewOscillatorFrequency(DTC_OscillatorType_SERDES, targetFreq))
	{
		for (auto& link : DTC_Links)
		{
			ResetSERDES(link, 1000);
		}
		ResetSERDES(DTC_Link_CFO, 1000);
		// ResetSERDES(DTC_Link_EVB, 1000);
	}
}

void DTCLib::DTC_Registers::SetTimingOscillatorClock(uint32_t freq)
{
	double targetFreq = freq;
	if (SetNewOscillatorFrequency(DTC_OscillatorType_Timing, targetFreq))
	{
		ResetSERDES(DTC_Link_CFO, 1000);
		// ResetSERDES(DTC_Link_EVB, 1000);
	}
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTimingSERDESOscillatorFrequency()
{
	auto form = CreateFormatter(DTC_Register_SERDESTimingCardOscillatorFrequency);
	form.description = "SERDES Timing Card Oscillator Reference Frequency";
	std::stringstream o;
	o << std::dec << ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_CFO);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMainBoardSERDESOscillatorFrequency()
{
	auto form = CreateFormatter(DTC_Register_SERDESMainBoardOscillatorFrequency);
	form.description = "SERDES Main Board Oscillator Reference Frequency";
	std::stringstream o;
	o << std::dec << ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_EVB);
	form.vals.push_back(o.str());
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESOscillatorControl()
{
	auto form = CreateFormatter(DTC_Register_SERDESOscillatorIICBusControl);
	form.description = "SERDES Oscillator IIC Bus Control";
	form.vals.push_back(std::string("Reset:  [") + (ReadSERDESOscillatorIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESOscillatorParameterLow()
{
	auto form = CreateFormatter(DTC_Register_SERDESOscillatorIICBusLow);
	form.description = "SERDES Oscillator IIC Bus Low";
	auto data = ReadRegister_(DTC_Register_SERDESOscillatorIICBusLow);
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
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESOscillatorParameterHigh()
{
	auto form = CreateFormatter(DTC_Register_SERDESOscillatorIICBusHigh);
	form.description = "SERDES Oscillator IIC Bus High";
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(DTC_Register_SERDESOscillatorIICBusHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(DTC_Register_SERDESOscillatorIICBusHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// DDR Oscillator Registers
uint32_t DTCLib::DTC_Registers::ReadDDROscillatorReferenceFrequency()
{
	return ReadRegister_(DTC_Register_DDROscillatorReferenceFrequency);
}
void DTCLib::DTC_Registers::SetDDROscillatorReferenceFrequency(uint32_t freq)
{
	WriteRegister_(freq, DTC_Register_DDROscillatorReferenceFrequency);
}
bool DTCLib::DTC_Registers::ReadDDROscillatorIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(DTC_Register_DDROscillatorIICBusControl));
	return dataSet[31];
}
void DTCLib::DTC_Registers::ResetDDROscillatorIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), DTC_Register_DDROscillatorIICBusControl);
	while (ReadDDROscillatorIICInterfaceReset())
	{
		usleep(1000);
	}
}
void DTCLib::DTC_Registers::WriteDDRIICInterface(DTC_IICDDRBusAddress device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, DTC_Register_DDROscillatorIICBusLow);
	WriteRegister_(0x1, DTC_Register_DDROscillatorIICBusHigh);
	while (ReadRegister_(DTC_Register_DDROscillatorIICBusHigh) == 0x1)
	{
		usleep(1000);
	}
}
uint8_t DTCLib::DTC_Registers::ReadDDRIICInterface(DTC_IICDDRBusAddress device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, DTC_Register_DDROscillatorIICBusLow);
	WriteRegister_(0x2, DTC_Register_DDROscillatorIICBusHigh);
	while (ReadRegister_(DTC_Register_DDROscillatorIICBusHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(DTC_Register_DDROscillatorIICBusLow);
	return static_cast<uint8_t>(data);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDROscillatorFrequency()
{
	auto form = CreateFormatter(DTC_Register_DDROscillatorReferenceFrequency);
	form.description = "DDR Oscillator Reference Frequency";
	std::stringstream o;
	o << std::dec << ReadDDROscillatorReferenceFrequency();
	form.vals.push_back(o.str());
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDROscillatorControl()
{
	auto form = CreateFormatter(DTC_Register_DDROscillatorIICBusControl);
	form.description = "DDR Oscillator IIC Bus Control";
	form.vals.push_back(std::string("Reset:  [") + (ReadDDROscillatorIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDROscillatorParameterLow()
{
	auto form = CreateFormatter(DTC_Register_DDROscillatorIICBusLow);
	form.description = "DDR Oscillator IIC Bus Low";
	auto data = ReadRegister_(DTC_Register_DDROscillatorIICBusLow);
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
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDROscillatorParameterHigh()
{
	auto form = CreateFormatter(DTC_Register_DDROscillatorIICBusHigh);
	form.description = "DDR Oscillator IIC Bus High";
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(DTC_Register_DDROscillatorIICBusHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(DTC_Register_DDROscillatorIICBusHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// Timestamp Preset Registers
void DTCLib::DTC_Registers::SetTimestampPreset(const DTC_Timestamp& preset)
{
	auto timestamp = preset.GetTimestamp();
	auto timestampLow = static_cast<uint32_t>(timestamp.to_ulong());
	timestamp >>= 32;
	auto timestampHigh = static_cast<uint16_t>(timestamp.to_ulong());

	WriteRegister_(timestampLow, DTC_Register_TimestampPreset0);
	WriteRegister_(timestampHigh, DTC_Register_TimestampPreset1);
}

DTCLib::DTC_Timestamp DTCLib::DTC_Registers::ReadTimestampPreset()
{
	auto timestampLow = ReadRegister_(DTC_Register_TimestampPreset0);
	DTC_Timestamp output;
	output.SetTimestamp(timestampLow, static_cast<uint16_t>(ReadRegister_(DTC_Register_TimestampPreset1)));
	return output;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTimestampPreset0()
{
	auto form = CreateFormatter(DTC_Register_TimestampPreset0);
	form.description = "Timestamp Preset 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_TimestampPreset0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTimestampPreset1()
{
	auto form = CreateFormatter(DTC_Register_TimestampPreset1);
	form.description = "Timestamp Preset 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_TimestampPreset1);
	form.vals.push_back(o.str());
	return form;
}

// Data Pending Timer Register
void DTCLib::DTC_Registers::SetDataPendingTimer(uint32_t timer)
{
	WriteRegister_(timer, DTC_Register_DataPendingTimer);
}

uint32_t DTCLib::DTC_Registers::ReadDataPendingTimer() { return ReadRegister_(DTC_Register_DataPendingTimer); }

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDataPendingTimer()
{
	auto form = CreateFormatter(DTC_Register_DataPendingTimer);
	form.description = "DMA Data Pending Timer";
	std::stringstream o;
	o << "0x" << std::hex << ReadDataPendingTimer();
	form.vals.push_back(o.str());
	return form;
}

// NUMROCs Register
void DTCLib::DTC_Registers::SetMaxROCNumber(const DTC_Link_ID& link, const uint8_t& lastRoc)
{
	std::bitset<32> linkRocs = ReadRegister_(DTC_Register_NUMROCs);
	auto numRocs = lastRoc + 1;
	linkRocs[link * 3] = numRocs & 1;
	linkRocs[link * 3 + 1] = ((numRocs & 2) >> 1) & 1;
	linkRocs[link * 3 + 2] = ((numRocs & 4) >> 2) & 1;
	WriteRegister_(linkRocs.to_ulong(), DTC_Register_NUMROCs);
}

uint8_t DTCLib::DTC_Registers::ReadLinkROCCount(const DTC_Link_ID& link)
{
	std::bitset<32> linkRocs = ReadRegister_(DTC_Register_NUMROCs);
	auto number = linkRocs[link * 3] + (linkRocs[link * 3 + 1] << 1) + (linkRocs[link * 3 + 2] << 2);
	return number;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatNUMROCs()
{
	auto form = CreateFormatter(DTC_Register_NUMROCs);
	form.description = "NUMROCs";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) +
							" NUMROCs:    " + std::to_string(ReadLinkROCCount(r)));
	}
	return form;
}

// FIFO Full Error Flags Registers
void DTCLib::DTC_Registers::ClearFIFOFullErrorFlags(const DTC_Link_ID& link)
{
	auto flags = ReadFIFOFullErrorFlags(link);
	std::bitset<32> data0 = 0;
	std::bitset<32> data1 = 0;
	std::bitset<32> data2 = 0;

	data0[link] = flags.OutputData;
	data0[link + 8] = flags.CFOLinkInput;
	data0[link + 16] = flags.ReadoutRequestOutput;
	data0[link + 24] = flags.DataRequestOutput;
	data1[link] = flags.OtherOutput;
	data1[link + 8] = flags.OutputDCS;
	data1[link + 16] = flags.OutputDCSStage2;
	data1[link + 24] = flags.DataInput;
	data2[link] = flags.DCSStatusInput;

	WriteRegister_(data0.to_ulong(), DTC_Register_FIFOFullErrorFlag0);
	WriteRegister_(data1.to_ulong(), DTC_Register_FIFOFullErrorFlag1);
	WriteRegister_(data2.to_ulong(), DTC_Register_FIFOFullErrorFlag2);
}

DTCLib::DTC_FIFOFullErrorFlags DTCLib::DTC_Registers::ReadFIFOFullErrorFlags(const DTC_Link_ID& link)
{
	std::bitset<32> data0 = ReadRegister_(DTC_Register_FIFOFullErrorFlag0);
	std::bitset<32> data1 = ReadRegister_(DTC_Register_FIFOFullErrorFlag1);
	std::bitset<32> data2 = ReadRegister_(DTC_Register_FIFOFullErrorFlag2);
	DTC_FIFOFullErrorFlags flags;

	flags.OutputData = data0[link];
	flags.CFOLinkInput = data0[link + 8];
	flags.ReadoutRequestOutput = data0[link + 16];
	flags.DataRequestOutput = data0[link + 24];
	flags.OtherOutput = data1[link];
	flags.OutputDCS = data1[link + 8];
	flags.OutputDCSStage2 = data1[link + 16];
	flags.DataInput = data1[link + 24];
	flags.DCSStatusInput = data2[link];

	return flags;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFIFOFullErrorFlag0()
{
	auto form = CreateFormatter(DTC_Register_FIFOFullErrorFlag0);
	form.description = "FIFO Full Error Flags 0";
	form.vals.push_back("       ([DataRequest, ReadoutRequest, CFOLink, OutputData])");
	for (auto r : DTC_Links)
	{
		auto re = ReadFIFOFullErrorFlags(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.DataRequestOutput ? "x" : " ") + "," +
							(re.ReadoutRequestOutput ? "x" : " ") + "," + (re.CFOLinkInput ? "x" : " ") + "," +
							(re.OutputData ? "x" : " ") + "]");
	}
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFIFOFullErrorFlag1()
{
	auto form = CreateFormatter(DTC_Register_FIFOFullErrorFlag1);
	form.description = "FIFO Full Error Flags 1";
	form.vals.push_back("       ([DataInput, OutputDCSStage2, OutputDCS, OtherOutput])");
	for (auto r : DTC_Links)
	{
		auto re = ReadFIFOFullErrorFlags(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.DataInput ? "x" : " ") + "," +
							(re.OutputDCSStage2 ? "x" : " ") + "," + (re.OutputDCS ? "x" : " ") + "," +
							(re.OtherOutput ? "x" : " ") + "]");
	}
	{
		auto ce = ReadFIFOFullErrorFlags(DTC_Link_CFO);
		form.vals.push_back(std::string("CFO:    [") + +(ce.DataInput ? "x" : " ") + "," +
							(ce.OutputDCSStage2 ? "x" : " ") + "," + (ce.OutputDCS ? "x" : " ") + "," +
							(ce.OtherOutput ? "x" : " ") + "]");
	}
	{
		auto ce = ReadFIFOFullErrorFlags(DTC_Link_EVB);
		form.vals.push_back(std::string("EVB:    [") + +(ce.DataInput ? "x" : " ") + "," +
							(ce.OutputDCSStage2 ? "x" : " ") + "," + (ce.OutputDCS ? "x" : " ") + "," +
							(ce.OtherOutput ? "x" : " ") + "]");
	}
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFIFOFullErrorFlag2()
{
	auto form = CreateFormatter(DTC_Register_FIFOFullErrorFlag2);
	form.description = "FIFO Full Error Flags 2";
	form.vals.push_back("       ([DCSStatusInput])");
	for (auto r : DTC_Links)
	{
		auto re = ReadFIFOFullErrorFlags(r);
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (re.DCSStatusInput ? "x" : " ") + "]");
	}
	{
		auto ce = ReadFIFOFullErrorFlags(DTC_Link_CFO);
		form.vals.push_back(std::string("CFO:    [") + (ce.DCSStatusInput ? "x" : " ") + "]");
	}
	{
		auto ce = ReadFIFOFullErrorFlags(DTC_Link_EVB);
		form.vals.push_back(std::string("EVB:    [") + (ce.DCSStatusInput ? "x" : " ") + "]");
	}
	return form;
}

// Receive Packet Error Register
void DTCLib::DTC_Registers::ClearPacketError(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_ReceivePacketError);
	data[static_cast<int>(link) + 8] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_ReceivePacketError);
}

bool DTCLib::DTC_Registers::ReadPacketError(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_ReceivePacketError);
	return data[static_cast<int>(link) + 8];
}

void DTCLib::DTC_Registers::ClearPacketCRCError(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_ReceivePacketError);
	data[static_cast<int>(link)] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_ReceivePacketError);
}

bool DTCLib::DTC_Registers::ReadPacketCRCError(const DTC_Link_ID& link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_ReceivePacketError);
	return data[static_cast<int>(link)];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketError()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketError);
	form.description = "Receive Packet Error";
	form.vals.push_back("       ([CRC, PacketError])");
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadPacketCRCError(r) ? "x" : " ") + "," +
							(ReadPacketError(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadPacketCRCError(DTC_Link_CFO) ? "x" : " ") + "," +
						(ReadPacketError(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadPacketCRCError(DTC_Link_EVB) ? "x" : " ") + "," +
						(ReadPacketError(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// CFO Emulation Timestamp Registers
void DTCLib::DTC_Registers::SetCFOEmulationTimestamp(const DTC_Timestamp& ts)
{
	auto timestamp = ts.GetTimestamp();
	auto timestampLow = static_cast<uint32_t>(timestamp.to_ulong());
	timestamp >>= 32;
	auto timestampHigh = static_cast<uint16_t>(timestamp.to_ulong());

	WriteRegister_(timestampLow, DTC_Register_CFOEmulationTimestampLow);
	WriteRegister_(timestampHigh, DTC_Register_CFOEmulationTimestampHigh);
}

DTCLib::DTC_Timestamp DTCLib::DTC_Registers::ReadCFOEmulationTimestamp()
{
	auto timestampLow = ReadRegister_(DTC_Register_CFOEmulationTimestampLow);
	DTC_Timestamp output;
	output.SetTimestamp(timestampLow, static_cast<uint16_t>(ReadRegister_(DTC_Register_CFOEmulationTimestampHigh)));
	return output;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationTimestampLow()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationTimestampLow);
	form.description = "CFO Emulation Timestamp Low";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_CFOEmulationTimestampLow);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationTimestampHigh()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationTimestampHigh);
	form.description = "CFO Emulation Timestamp High";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_CFOEmulationTimestampHigh);
	form.vals.push_back(o.str());
	return form;
}

// CFO Emulation Request Interval Register
void DTCLib::DTC_Registers::SetCFOEmulationRequestInterval(uint32_t interval)
{
	WriteRegister_(interval, DTC_Register_CFOEmulationRequestInterval);
}

uint32_t DTCLib::DTC_Registers::ReadCFOEmulationRequestInterval()
{
	return ReadRegister_(DTC_Register_CFOEmulationRequestInterval);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationRequestInterval()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationRequestInterval);
	form.description = "CFO Emu. Request Interval";
	std::stringstream o;
	o << "0x" << std::hex << ReadCFOEmulationRequestInterval();
	form.vals.push_back(o.str());
	return form;
}

// CFO Emulation Number of Requests Register
void DTCLib::DTC_Registers::SetCFOEmulationNumRequests(uint32_t numRequests)
{
	WriteRegister_(numRequests, DTC_Register_CFOEmulationNumRequests);
}

uint32_t DTCLib::DTC_Registers::ReadCFOEmulationNumRequests()
{
	return ReadRegister_(DTC_Register_CFOEmulationNumRequests);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationNumRequests()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationNumRequests);
	form.description = "CFO Emulator Number Requests";
	std::stringstream o;
	o << "0x" << std::hex << ReadCFOEmulationNumRequests();
	form.vals.push_back(o.str());
	return form;
}

// CFO Emulation Number of Packets Registers
void DTCLib::DTC_Registers::SetCFOEmulationNumPackets(const DTC_Link_ID& link, uint16_t numPackets)
{
	uint16_t data = numPackets & 0x7FF;
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
		case DTC_Link_1:
			reg = DTC_Register_CFOEmulationNumPacketsLinks10;
			break;
		case DTC_Link_2:
		case DTC_Link_3:
			reg = DTC_Register_CFOEmulationNumPacketsLinks32;
			break;
		case DTC_Link_4:
		case DTC_Link_5:
			reg = DTC_Register_CFOEmulationNumPacketsLinks54;
			break;
		default:
			return;
	}

	auto regval = ReadRegister_(reg);
	auto upper = (regval & 0xFFFF0000) >> 16;
	auto lower = regval & 0x0000FFFF;
	if (link == DTC_Link_0 || link == DTC_Link_2 || link == DTC_Link_4)
	{
		lower = data;
	}
	else
	{
		upper = data;
	}
	WriteRegister_((upper << 16) + lower, reg);
}

uint16_t DTCLib::DTC_Registers::ReadCFOEmulationNumPackets(const DTC_Link_ID& link)
{
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
		case DTC_Link_1:
			reg = DTC_Register_CFOEmulationNumPacketsLinks10;
			break;
		case DTC_Link_2:
		case DTC_Link_3:
			reg = DTC_Register_CFOEmulationNumPacketsLinks32;
			break;
		case DTC_Link_4:
		case DTC_Link_5:
			reg = DTC_Register_CFOEmulationNumPacketsLinks54;
			break;
		default:
			return 0;
	}

	auto regval = ReadRegister_(reg);
	auto upper = (regval & 0xFFFF0000) >> 16;
	auto lower = regval & 0x0000FFFF;
	if (link == DTC_Link_0 || link == DTC_Link_2 || link == DTC_Link_4)
	{
		return static_cast<uint16_t>(lower);
	}
	return static_cast<uint16_t>(upper);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationNumPacketsLink01()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationNumPacketsLinks10);
	form.description = "CFO Emulator Num Packets R0,1";
	std::stringstream o;
	o << "Link 0: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_0);
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Link 1: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationNumPacketsLink23()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationNumPacketsLinks32);
	form.description = "CFO Emulator Num Packets R2,3";
	std::stringstream o;
	o << "Link 2: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_2);
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Link 3: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationNumPacketsLink45()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationNumPacketsLinks54);
	form.description = "CFO Emulator Num Packets R4,5";
	std::stringstream o;
	o << "Link 4: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_4);
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Link 5: 0x" << std::hex << ReadCFOEmulationNumPackets(DTC_Link_5);
	form.vals.push_back(o.str());
	return form;
}

// CFO Emulation Number of Null Heartbeats Register
void DTCLib::DTC_Registers::SetCFOEmulationNumNullHeartbeats(const uint32_t& count)
{
	WriteRegister_(count, DTC_Register_CFOEmulationNumNullHeartbeats);
}

uint32_t DTCLib::DTC_Registers::ReadCFOEmulationNumNullHeartbeats()
{
	return ReadRegister_(DTC_Register_CFOEmulationNumNullHeartbeats);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationNumNullHeartbeats()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationNumNullHeartbeats);
	form.description = "CFO Emulator Num Null Heartbeats";
	form.vals.push_back(std::to_string(ReadCFOEmulationNumNullHeartbeats()));
	return form;
}

// CFO Emulation Event Mode Bytes Registers
void DTCLib::DTC_Registers::SetCFOEmulationModeByte(const uint8_t& byteNum, uint8_t data)
{
	DTC_Register reg;
	if (byteNum == 0 || byteNum == 1 || byteNum == 2 || byteNum == 3)
	{
		reg = DTC_Register_CFOEmulationEventMode1;
	}
	else if (byteNum == 4 || byteNum == 5)
	{
		reg = DTC_Register_CFOEmulationEventMode2;
	}
	else
	{
		return;
	}
	auto regVal = ReadRegister_(reg);

	switch (byteNum)
	{
		case 0:
			regVal = (regVal & 0xFFFFFF00) + data;
			break;
		case 1:
			regVal = (regVal & 0xFFFF00FF) + (data << 8);
			break;
		case 2:
			regVal = (regVal & 0xFF00FFFF) + (data << 16);
			break;
		case 3:
			regVal = (regVal & 0x00FFFFFF) + (data << 24);
			break;
		case 4:
			regVal = (regVal & 0xFF00) + data;
			break;
		case 5:
			regVal = (regVal & 0x00FF) + (data << 8);
			break;
		default:
			return;
	}

	WriteRegister_(regVal, reg);
}

uint8_t DTCLib::DTC_Registers::ReadCFOEmulationModeByte(const uint8_t& byteNum)
{
	DTC_Register reg;
	if (byteNum == 0 || byteNum == 1 || byteNum == 2 || byteNum == 3)
	{
		reg = DTC_Register_CFOEmulationEventMode1;
	}
	else if (byteNum == 4 || byteNum == 5)
	{
		reg = DTC_Register_CFOEmulationEventMode2;
	}
	else
	{
		return 0;
	}
	auto regVal = ReadRegister_(reg);

	switch (byteNum)
	{
		case 0:
			return static_cast<uint8_t>(regVal & 0xFF);
		case 1:
			return static_cast<uint8_t>((regVal & 0xFF00) >> 8);
		case 2:
			return static_cast<uint8_t>((regVal & 0xFF0000) >> 16);
		case 3:
			return static_cast<uint8_t>((regVal & 0xFF000000) >> 24);
		case 4:
			return static_cast<uint8_t>(regVal & 0xFF);
		case 5:
			return static_cast<uint8_t>((regVal & 0xFF00) >> 8);
		default:
			return 0;
	}
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationModeBytes03()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationEventMode1);
	form.description = "CFO Emulation Event Mode Bytes 0-3";
	std::ostringstream o;
	o << "Byte 0: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(0));
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Byte 1: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(1));
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Byte 2: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(2));
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Byte 3: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(3));
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationModeBytes45()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationEventMode2);
	form.description = "CFO Emulation Event Mode Bytes 4-5";
	std::ostringstream o;
	o << "Byte 4: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(4));
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "Byte 5: 0x" << std::hex << static_cast<int>(ReadCFOEmulationModeByte(5));
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	return form;
}

// CFO Emulation Debug Packet Type Register

void DTCLib::DTC_Registers::EnableDebugPacketMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_CFOEmulationDebugPacketType);
	data[16] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_CFOEmulationDebugPacketType);
}

void DTCLib::DTC_Registers::DisableDebugPacketMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_CFOEmulationDebugPacketType);
	data[16] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_CFOEmulationDebugPacketType);
}

bool DTCLib::DTC_Registers::ReadDebugPacketMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_CFOEmulationDebugPacketType);
	return data[16];
}

void DTCLib::DTC_Registers::SetCFOEmulationDebugType(DTC_DebugType type)
{
	std::bitset<32> data = type & 0xF;
	data[16] = ReadDebugPacketMode();
	WriteRegister_(data.to_ulong(), DTC_Register_CFOEmulationDebugPacketType);
}

DTCLib::DTC_DebugType DTCLib::DTC_Registers::ReadCFOEmulationDebugType()
{
	return static_cast<DTC_DebugType>(0xFFFF & ReadRegister_(DTC_Register_CFOEmulationDebugPacketType));
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOEmulationDebugPacketType()
{
	auto form = CreateFormatter(DTC_Register_CFOEmulationDebugPacketType);
	form.description = "CFO Emulation Debug Packet Type";
	form.vals.push_back(std::string("Debug Mode: [") + (ReadDebugPacketMode() ? "x" : " ") + "]");
	std::stringstream o;
	o << "Debug Packet Type: 0x" << std::hex << ReadCFOEmulationDebugType();
	form.vals.push_back(o.str());
	return form;
}

// RX Packet Count Error Flags Register
bool DTCLib::DTC_Registers::ReadRXPacketCountErrorFlags(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_RXPacketCountErrorFlags);
	return dataSet[link];
}

void DTCLib::DTC_Registers::ClearRXPacketCountErrorFlags(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet;
	dataSet[link] = true;
	WriteRegister_(dataSet.to_ulong(), DTC_Register_RXPacketCountErrorFlags);
}

void DTCLib::DTC_Registers::ClearRXPacketCountErrorFlags()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_RXPacketCountErrorFlags);
	WriteRegister_(dataSet.to_ulong(), DTC_Register_RXPacketCountErrorFlags);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRXPacketCountErrorFlags()
{
	auto form = CreateFormatter(DTC_Register_RXPacketCountErrorFlags);
	form.description = "RX Packet Count Error Flags";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" +
							(ReadRXPacketCountErrorFlags(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadRXPacketCountErrorFlags(DTC_Link_CFO) ? "x" : " ") + "]");
	form.vals.push_back(std::string("EVB:    [") + (ReadRXPacketCountErrorFlags(DTC_Link_EVB) ? "x" : " ") + "]");
	return form;
}

// Detector Emulator DMA Count Register
void DTCLib::DTC_Registers::SetDetectorEmulationDMACount(uint32_t count)
{
	WriteRegister_(count, DTC_Register_DetEmulationDMACount);
}

uint32_t DTCLib::DTC_Registers::ReadDetectorEmulationDMACount()
{
	return ReadRegister_(DTC_Register_DetEmulationDMACount);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDetectorEmulationDMACount()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationDMACount);
	form.description = "DetEmu DMA Count";
	std::stringstream o;
	o << "0x" << std::hex << ReadDetectorEmulationDMACount();
	form.vals.push_back(o.str());
	return form;
}

// Detector Emulator DMA Delay Counter Register
void DTCLib::DTC_Registers::SetDetectorEmulationDMADelayCount(uint32_t count)
{
	WriteRegister_(count, DTC_Register_DetEmulationDelayCount);
}

uint32_t DTCLib::DTC_Registers::ReadDetectorEmulationDMADelayCount()
{
	return ReadRegister_(DTC_Register_DetEmulationDelayCount);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDetectorEmulationDMADelayCount()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationDelayCount);
	form.description = "DetEmu DMA Delay Count";
	std::stringstream o;
	o << "0x" << std::hex << ReadDetectorEmulationDMADelayCount();
	form.vals.push_back(o.str());
	return form;
}

void DTCLib::DTC_Registers::EnableDetectorEmulatorMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl0);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DetEmulationControl0);
}

void DTCLib::DTC_Registers::DisableDetectorEmulatorMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl0);
	data[0] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_DetEmulationControl0);
}

bool DTCLib::DTC_Registers::ReadDetectorEmulatorMode()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl0);
	return data[0];
}

void DTCLib::DTC_Registers::EnableDetectorEmulator()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl0);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DetEmulationControl0);
}

void DTCLib::DTC_Registers::DisableDetectorEmulator()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl1);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_DetEmulationControl1);
}

bool DTCLib::DTC_Registers::ReadDetectorEmulatorEnable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl0);
	return data[1];
}

bool DTCLib::DTC_Registers::ReadDetectorEmulatorEnableClear()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_DetEmulationControl1);
	return data[1];
}

void DTCLib::DTC_Registers::ClearDetectorEmulatorInUse()
{
	DisableDetectorEmulator();
	DisableDetectorEmulatorMode();
	ResetDDRWriteAddress();
	ResetDDRReadAddress();
	SetDDRDataLocalStartAddress(0);
	SetDDRDataLocalEndAddress(0x7000000);
	usingDetectorEmulator_ = false;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDetectorEmulationControl0()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationControl0);
	form.description = "Detector Emulation Control 0";
	form.vals.push_back(std::string("Detector Emulation Enable: [") + (ReadDetectorEmulatorEnable() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Detector Emulation Mode:   [") + (ReadDetectorEmulatorMode() ? "x" : " ") + "]");
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDetectorEmulationControl1()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationControl1);
	form.description = "Detector Emulation Control 1";
	form.vals.push_back(std::string("Detector Emulation Enable Clear: [") +
						(ReadDetectorEmulatorEnableClear() ? "x" : " ") + "]");
	return form;
}

// DDR Event Data Local Start Address Register
void DTCLib::DTC_Registers::SetDDRDataLocalStartAddress(uint32_t address)
{
	WriteRegister_(address, DTC_Register_DetEmulationDataStartAddress);
}

uint32_t DTCLib::DTC_Registers::ReadDDRDataLocalStartAddress()
{
	return ReadRegister_(DTC_Register_DetEmulationDataStartAddress);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRDataLocalStartAddress()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationDataStartAddress);
	form.description = "DDR Event Data Local Start Address";
	std::stringstream o;
	o << "0x" << std::hex << ReadDDRDataLocalStartAddress();
	form.vals.push_back(o.str());
	return form;
}

// DDR Event Data Local End Address Register
void DTCLib::DTC_Registers::SetDDRDataLocalEndAddress(uint32_t address)
{
	WriteRegister_(address, DTC_Register_DetEmulationDataEndAddress);
}

uint32_t DTCLib::DTC_Registers::ReadDDRDataLocalEndAddress()
{
	return ReadRegister_(DTC_Register_DetEmulationDataEndAddress);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRDataLocalEndAddress()
{
	auto form = CreateFormatter(DTC_Register_DetEmulationDataEndAddress);
	form.description = "DDR Event Data Local End Address";
	std::stringstream o;
	o << "0x" << std::hex << ReadDDRDataLocalEndAddress();
	form.vals.push_back(o.str());
	return form;
}

// ROC DRP Sync Error Register
bool DTCLib::DTC_Registers::ReadROCDRPSyncErrors(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ROCDRPDataSyncError);
	return dataSet[link];
}

void DTCLib::DTC_Registers::ClearROCDRPSyncErrors(const DTC_Link_ID& link)
{
	std::bitset<32> dataSet;
	dataSet[link] = true;
	WriteRegister_(dataSet.to_ulong(), DTC_Register_ROCDRPDataSyncError);
}

void DTCLib::DTC_Registers::ClearROCDRPSyncErrors()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_ROCDRPDataSyncError);
	WriteRegister_(dataSet.to_ulong(), DTC_Register_ROCDRPDataSyncError);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatROCDRPSyncError()
{
	auto form = CreateFormatter(DTC_Register_ROCDRPDataSyncError);
	form.description = "RX Packet Count Error Flags";
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" +
							(ReadRXPacketCountErrorFlags(r) ? "x" : " ") + "]");
	}
	return form;
}

uint32_t DTCLib::DTC_Registers::ReadEthernetPayloadSize()
{
	return ReadRegister_(DTC_Register_EthernetFramePayloadSize);
}

void DTCLib::DTC_Registers::SetEthernetPayloadSize(uint32_t size)
{
	if (size > 1492)
	{
		size = 1492;
	}
	WriteRegister_(size, DTC_Register_EthernetFramePayloadSize);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatEthernetPayloadSize()
{
	auto form = CreateFormatter(DTC_Register_EthernetFramePayloadSize);
	form.description = "Ethernet Frame Payload Max Size";
	std::stringstream o;
	o << std::dec << ReadEthernetPayloadSize() << " bytes";
	form.vals.push_back(o.str());
	return form;
}

// SERDES Counter Registers
void DTCLib::DTC_Registers::ClearReceiveByteCount(const DTC_Link_ID& link)
{
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
			reg = DTC_Register_ReceiveByteCountDataLink0;
			break;
		case DTC_Link_1:
			reg = DTC_Register_ReceiveByteCountDataLink1;
			break;
		case DTC_Link_2:
			reg = DTC_Register_ReceiveByteCountDataLink2;
			break;
		case DTC_Link_3:
			reg = DTC_Register_ReceiveByteCountDataLink3;
			break;
		case DTC_Link_4:
			reg = DTC_Register_ReceiveByteCountDataLink4;
			break;
		case DTC_Link_5:
			reg = DTC_Register_ReceiveByteCountDataLink5;
			break;
		case DTC_Link_CFO:
			reg = DTC_Register_ReceiveByteCountDataCFO;
			break;
		case DTC_Link_EVB:
			reg = DTC_Register_ReceiveByteCountDataEVB;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t DTCLib::DTC_Registers::ReadReceiveByteCount(const DTC_Link_ID& link)
{
	switch (link)
	{
		case DTC_Link_0:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink0);
		case DTC_Link_1:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink1);
		case DTC_Link_2:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink2);
		case DTC_Link_3:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink3);
		case DTC_Link_4:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink4);
		case DTC_Link_5:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataLink5);
		case DTC_Link_CFO:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataCFO);
		case DTC_Link_EVB:
			return ReadRegister_(DTC_Register_ReceiveByteCountDataEVB);
		default:
			return 0;
	}
}

void DTCLib::DTC_Registers::ClearReceivePacketCount(const DTC_Link_ID& link)
{
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
			reg = DTC_Register_ReceivePacketCountDataLink0;
			break;
		case DTC_Link_1:
			reg = DTC_Register_ReceivePacketCountDataLink1;
			break;
		case DTC_Link_2:
			reg = DTC_Register_ReceivePacketCountDataLink2;
			break;
		case DTC_Link_3:
			reg = DTC_Register_ReceivePacketCountDataLink3;
			break;
		case DTC_Link_4:
			reg = DTC_Register_ReceivePacketCountDataLink4;
			break;
		case DTC_Link_5:
			reg = DTC_Register_ReceivePacketCountDataLink5;
			break;
		case DTC_Link_CFO:
			reg = DTC_Register_ReceivePacketCountDataCFO;
			break;
		case DTC_Link_EVB:
			reg = DTC_Register_ReceivePacketCountDataEVB;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t DTCLib::DTC_Registers::ReadReceivePacketCount(const DTC_Link_ID& link)
{
	switch (link)
	{
		case DTC_Link_0:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink0);
		case DTC_Link_1:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink1);
		case DTC_Link_2:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink2);
		case DTC_Link_3:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink3);
		case DTC_Link_4:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink4);
		case DTC_Link_5:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataLink5);
		case DTC_Link_CFO:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataCFO);
		case DTC_Link_EVB:
			return ReadRegister_(DTC_Register_ReceivePacketCountDataEVB);
		default:
			return 0;
	}
}

void DTCLib::DTC_Registers::ClearTransmitByteCount(const DTC_Link_ID& link)
{
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
			reg = DTC_Register_TransmitByteCountDataLink0;
			break;
		case DTC_Link_1:
			reg = DTC_Register_TransmitByteCountDataLink1;
			break;
		case DTC_Link_2:
			reg = DTC_Register_TransmitByteCountDataLink2;
			break;
		case DTC_Link_3:
			reg = DTC_Register_TransmitByteCountDataLink3;
			break;
		case DTC_Link_4:
			reg = DTC_Register_TransmitByteCountDataLink4;
			break;
		case DTC_Link_5:
			reg = DTC_Register_TransmitByteCountDataLink5;
			break;
		case DTC_Link_CFO:
			reg = DTC_Register_TransmitByteCountDataCFO;
			break;
		case DTC_Link_EVB:
			reg = DTC_Register_TransmitByteCountDataEVB;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t DTCLib::DTC_Registers::ReadTransmitByteCount(const DTC_Link_ID& link)
{
	switch (link)
	{
		case DTC_Link_0:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink0);
		case DTC_Link_1:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink1);
		case DTC_Link_2:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink2);
		case DTC_Link_3:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink3);
		case DTC_Link_4:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink4);
		case DTC_Link_5:
			return ReadRegister_(DTC_Register_TransmitByteCountDataLink5);
		case DTC_Link_CFO:
			return ReadRegister_(DTC_Register_TransmitByteCountDataCFO);
		case DTC_Link_EVB:
			return ReadRegister_(DTC_Register_TransmitByteCountDataEVB);
		default:
			return 0;
	}
}

void DTCLib::DTC_Registers::ClearTransmitPacketCount(const DTC_Link_ID& link)
{
	DTC_Register reg;
	switch (link)
	{
		case DTC_Link_0:
			reg = DTC_Register_TransmitPacketCountDataLink0;
			break;
		case DTC_Link_1:
			reg = DTC_Register_TransmitPacketCountDataLink1;
			break;
		case DTC_Link_2:
			reg = DTC_Register_TransmitPacketCountDataLink2;
			break;
		case DTC_Link_3:
			reg = DTC_Register_TransmitPacketCountDataLink3;
			break;
		case DTC_Link_4:
			reg = DTC_Register_TransmitPacketCountDataLink4;
			break;
		case DTC_Link_5:
			reg = DTC_Register_TransmitPacketCountDataLink5;
			break;
		case DTC_Link_CFO:
			reg = DTC_Register_TransmitPacketCountDataCFO;
			break;
		case DTC_Link_EVB:
			reg = DTC_Register_TransmitPacketCountDataEVB;
			break;
		default:
			return;
	}
	WriteRegister_(0, reg);
}

uint32_t DTCLib::DTC_Registers::ReadTransmitPacketCount(const DTC_Link_ID& link)
{
	switch (link)
	{
		case DTC_Link_0:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink0);
		case DTC_Link_1:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink1);
		case DTC_Link_2:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink2);
		case DTC_Link_3:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink3);
		case DTC_Link_4:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink4);
		case DTC_Link_5:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataLink5);
		case DTC_Link_CFO:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataCFO);
		case DTC_Link_EVB:
			return ReadRegister_(DTC_Register_TransmitPacketCountDataEVB);
		default:
			return 0;
	}
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing0()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink0);
	form.description = "Receive Byte Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing1()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink1);
	form.description = "Receive Byte Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing2()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink2);
	form.description = "Receive Byte Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing3()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink3);
	form.description = "Receive Byte Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing4()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink4);
	form.description = "Receive Byte Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountRing5()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataLink5);
	form.description = "Receive Byte Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountCFO()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataCFO);
	form.description = "Receive Byte Count: CFO";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_CFO);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceiveByteCountEVB()
{
	auto form = CreateFormatter(DTC_Register_ReceiveByteCountDataEVB);
	form.description = "Receive Byte Count: EVB";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceiveByteCount(DTC_Link_EVB);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing0()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink0);
	form.description = "Receive Packet Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing1()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink1);
	form.description = "Receive Packet Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing2()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink2);
	form.description = "Receive Packet Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing3()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink3);
	form.description = "Receive Packet Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing4()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink4);
	form.description = "Receive Packet Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountRing5()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataLink5);
	form.description = "Receive Packet Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountCFO()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataCFO);
	form.description = "Receive Packet Count: CFO";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_CFO);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatReceivePacketCountEVB()
{
	auto form = CreateFormatter(DTC_Register_ReceivePacketCountDataEVB);
	form.description = "Receive Packet Count: EVB";
	std::stringstream o;
	o << "0x" << std::hex << ReadReceivePacketCount(DTC_Link_EVB);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing0()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink0);
	form.description = "Transmit Byte Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing1()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink1);
	form.description = "Transmit Byte Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing2()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink2);
	form.description = "Transmit Byte Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing3()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink3);
	form.description = "Transmit Byte Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing4()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink4);
	form.description = "Transmit Byte Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountRing5()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataLink5);
	form.description = "Transmit Byte Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountCFO()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataCFO);
	form.description = "Transmit Byte Count: CFO";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_CFO);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTramsitByteCountEVB()
{
	auto form = CreateFormatter(DTC_Register_TransmitByteCountDataEVB);
	form.description = "Transmit Byte Count: EVB";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitByteCount(DTC_Link_EVB);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing0()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink0);
	form.description = "Transmit Packet Count: Link 0";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_0);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing1()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink1);
	form.description = "Transmit Packet Count: Link 1";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing2()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink2);
	form.description = "Transmit Packet Count: Link 2";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing3()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink3);
	form.description = "Transmit Packet Count: Link 3";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_3);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing4()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink4);
	form.description = "Transmit Packet Count: Link 4";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_4);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountRing5()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataLink5);
	form.description = "Transmit Packet Count: Link 5";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_5);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountCFO()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataCFO);
	form.description = "Transmit Packet Count: CFO";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_CFO);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatTransmitPacketCountEVB()
{
	auto form = CreateFormatter(DTC_Register_TransmitPacketCountDataEVB);
	form.description = "Transmit Packet Count: EVB";
	std::stringstream o;
	o << "0x" << std::hex << ReadTransmitPacketCount(DTC_Link_EVB);
	form.vals.push_back(o.str());
	return form;
}

// Firefly TX IIC Registers
bool DTCLib::DTC_Registers::ReadFireflyTXIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(DTC_Register_FireflyTXIICBusControl));
	return dataSet[31];
}
void DTCLib::DTC_Registers::ResetFireflyTXIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), DTC_Register_FireflyTXIICBusControl);
	while (ReadFireflyTXIICInterfaceReset())
	{
		usleep(1000);
	}
}
void DTCLib::DTC_Registers::WriteFireflyTXIICInterface(uint8_t device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, DTC_Register_FireflyTXIICBusConfigLow);
	WriteRegister_(0x1, DTC_Register_FireflyTXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyTXIICBusConfigHigh) == 0x1)
	{
		usleep(1000);
	}
}
uint8_t DTCLib::DTC_Registers::ReadFireflyTXIICInterface(uint8_t device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, DTC_Register_FireflyTXIICBusConfigLow);
	WriteRegister_(0x2, DTC_Register_FireflyTXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyTXIICBusConfigHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(DTC_Register_FireflyTXIICBusConfigLow);
	return static_cast<uint8_t>(data);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXIICControl()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXIICBusControl);
	form.description = "TX Firefly IIC Bus Control";
	form.vals.push_back(std::string("Reset:  [") + (ReadFireflyTXIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXIICParameterLow()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXIICBusConfigLow);
	form.description = "TX Firefly IIC Bus Low";
	auto data = ReadRegister_(DTC_Register_FireflyTXIICBusConfigLow);
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
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXIICParameterHigh()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXIICBusConfigHigh);
	form.description = "TX Firefly IIC Bus High";
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(DTC_Register_FireflyTXIICBusConfigHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(DTC_Register_FireflyTXIICBusConfigHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// Firefly RX IIC Registers
bool DTCLib::DTC_Registers::ReadFireflyRXIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(DTC_Register_FireflyRXIICBusControl));
	return dataSet[31];
}
void DTCLib::DTC_Registers::ResetFireflyRXIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), DTC_Register_FireflyRXIICBusControl);
	while (ReadFireflyRXIICInterfaceReset())
	{
		usleep(1000);
	}
}
void DTCLib::DTC_Registers::WriteFireflyRXIICInterface(uint8_t device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, DTC_Register_FireflyRXIICBusConfigLow);
	WriteRegister_(0x1, DTC_Register_FireflyRXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyRXIICBusConfigHigh) == 0x1)
	{
		usleep(1000);
	}
}
uint8_t DTCLib::DTC_Registers::ReadFireflyRXIICInterface(uint8_t device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, DTC_Register_FireflyRXIICBusConfigLow);
	WriteRegister_(0x2, DTC_Register_FireflyRXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyRXIICBusConfigHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(DTC_Register_FireflyRXIICBusConfigLow);
	return static_cast<uint8_t>(data);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyRXIICControl()
{
	auto form = CreateFormatter(DTC_Register_FireflyRXIICBusControl);
	form.description = "RX Firefly IIC Bus Control";
	form.vals.push_back(std::string("Reset:  [") + (ReadFireflyRXIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyRXIICParameterLow()
{
	auto form = CreateFormatter(DTC_Register_FireflyRXIICBusConfigLow);
	form.description = "RX Firefly IIC Bus Low";
	auto data = ReadRegister_(DTC_Register_FireflyRXIICBusConfigLow);
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
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyRXIICParameterHigh()
{
	auto form = CreateFormatter(DTC_Register_FireflyRXIICBusConfigHigh);
	form.description = "RX Firefly IIC Bus High";
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(DTC_Register_FireflyRXIICBusConfigHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(DTC_Register_FireflyRXIICBusConfigHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// Firefly TXRX IIC Registers
bool DTCLib::DTC_Registers::ReadFireflyTXRXIICInterfaceReset()
{
	auto dataSet = std::bitset<32>(ReadRegister_(DTC_Register_FireflyTXRXIICBusControl));
	return dataSet[31];
}
void DTCLib::DTC_Registers::ResetFireflyTXRXIICInterface()
{
	auto bs = std::bitset<32>();
	bs[31] = 1;
	WriteRegister_(bs.to_ulong(), DTC_Register_FireflyTXRXIICBusControl);
	while (ReadFireflyTXRXIICInterfaceReset())
	{
		usleep(1000);
	}
}
void DTCLib::DTC_Registers::WriteFireflyTXRXIICInterface(uint8_t device, uint8_t address, uint8_t data)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16) + (data << 8);
	WriteRegister_(reg_data, DTC_Register_FireflyTXRXIICBusConfigLow);
	WriteRegister_(0x1, DTC_Register_FireflyTXRXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigHigh) == 0x1)
	{
		usleep(1000);
	}
}
uint8_t DTCLib::DTC_Registers::ReadFireflyTXRXIICInterface(uint8_t device, uint8_t address)
{
	uint32_t reg_data = (static_cast<uint8_t>(device) << 24) + (address << 16);
	WriteRegister_(reg_data, DTC_Register_FireflyTXRXIICBusConfigLow);
	WriteRegister_(0x2, DTC_Register_FireflyTXRXIICBusConfigHigh);
	while (ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigHigh) == 0x2)
	{
		usleep(1000);
	}
	auto data = ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigLow);
	return static_cast<uint8_t>(data);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXRXIICControl()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXRXIICBusControl);
	form.description = "TXRX Firefly IIC Bus Control";
	form.vals.push_back(std::string("Reset:  [") + (ReadFireflyTXRXIICInterfaceReset() ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXRXIICParameterLow()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXRXIICBusConfigLow);
	form.description = "TXRX Firefly IIC Bus Low";
	auto data = ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigLow);
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
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyTXRXIICParameterHigh()
{
	auto form = CreateFormatter(DTC_Register_FireflyTXRXIICBusConfigHigh);
	form.description = "TXRX Firefly IIC Bus High";
	form.vals.push_back(std::string("Write:  [") +
						(ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigHigh) & 0x1 ? "x" : " ") + "]");
	form.vals.push_back(std::string("Read:   [") +
						(ReadRegister_(DTC_Register_FireflyTXRXIICBusConfigHigh) & 0x2 ? "x" : " ") + "]");
	return form;
}

// DDR Memory Flags Registers
DTCLib::DTC_DDRFlags DTCLib::DTC_Registers::ReadDDRFlags(uint8_t buffer_id)
{
	DTC_DDRFlags output;
	if (buffer_id >= 64) return output;

	auto lbf = ReadDDRLinkBufferFullFlags();
	output.InputFragmentBufferFull = lbf[buffer_id];
	auto lbfe = ReadDDRLinkBufferFullErrorFlags();
	output.InputFragmentBufferFullError = lbfe[buffer_id];
	auto lbe = ReadDDRLinkBufferEmptyFlags();
	output.InputFragmentBufferEmpty = lbe[buffer_id];
	auto lbhf = ReadDDRLinkBufferHalfFullFlags();
	output.InputFragmentBufferHalfFull = lbhf[buffer_id];
	auto ebf = ReadDDREventBuilderBufferFullFlags();
	output.OutputEventBufferFull = ebf[buffer_id];
	auto ebfe = ReadDDREventBuilderBufferFullErrorFlags();
	output.OutputEventBufferFullError = ebfe[buffer_id];
	auto ebe = ReadDDREventBuilderBufferEmptyFlags();
	output.OutputEventBufferEmpty = ebe[buffer_id];
	auto ebhf = ReadDDREventBuilderBufferHalfFullFlags();
	output.OutputEventBufferHalfFull = ebhf[buffer_id];

	return output;
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDRLinkBufferFullFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_DDRLinkBufferFullFlags1) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_DDRLinkBufferFullFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDRLinkBufferFullErrorFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_DDRLinkBufferFullErrorFlags1) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_DDRLinkBufferFullErrorFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDRLinkBufferEmptyFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_DDRLinkBufferEmptyFlags1) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_DDRLinkBufferEmptyFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDRLinkBufferHalfFullFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_DDRLinkBufferHalfFullFlags1) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_DDRLinkBufferHalfFullFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDREventBuilderBufferFullFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_EventBuilderBufferFullFlags2) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_EventBuilderBufferFullFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDREventBuilderBufferFullErrorFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_EventBuilderBufferFullErrorFlags2) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_EventBuilderBufferFullErrorFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDREventBuilderBufferEmptyFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_EventBuilderBufferEmptyFlags2) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_EventBuilderBufferEmptyFlags2)) << 32);
	return std::bitset<64>(flags);
}

std::bitset<64> DTCLib::DTC_Registers::ReadDDREventBuilderBufferHalfFullFlags()
{
	uint64_t flags = ReadRegister_(DTC_Register_EventBuilderBufferHalfFullFlags2) +
					 (static_cast<uint64_t>(ReadRegister_(DTC_Register_EventBuilderBufferHalfFullFlags2)) << 32);
	return std::bitset<64>(flags);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferFullFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferFullFlags1);
	form.description = "DDR Link Buffer Full Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferFullFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferFullErrorFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferFullErrorFlags1);
	form.description = "DDR Link Buffer Full Error Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferFullErrorFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferEmptyFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferEmptyFlags1);
	form.description = "DDR Link Buffer Empty Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferEmptyFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferHalfFullFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferHalfFullFlags1);
	form.description = "DDR Link Buffer Half Full Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferHalfFullFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferFullFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferFullFlags1);
	form.description = "DDR Event Builder Buffer Full Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferFullFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferFullErrorFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferFullErrorFlags1);
	form.description = "DDR Event Builder Buffer Full Error Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferFullErrorFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferEmptyFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferEmptyFlags1);
	form.description = "DDR Event Builder Buffer Empty Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferEmptyFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferHalfFullFlagsLow()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferHalfFullFlags1);
	form.description = "DDR Event Builder Buffer Half Full Flags (0-31)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferHalfFullFlags1);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferFullFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferFullFlags2);
	form.description = "DDR Link Buffer Full Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferFullFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferFullErrorFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferFullErrorFlags2);
	form.description = "DDR Link Buffer Full Error Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferFullErrorFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferEmptyFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferEmptyFlags2);
	form.description = "DDR Link Buffer Empty Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferEmptyFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDRLinkBufferHalfFullFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_DDRLinkBufferHalfFullFlags2);
	form.description = "DDR Link Buffer Half Full Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_DDRLinkBufferHalfFullFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferFullFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferFullFlags2);
	form.description = "DDR Event Builder Buffer Full Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferFullFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferFullErrorFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferFullErrorFlags2);
	form.description = "DDR Event Builder Buffer Full Error Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferFullErrorFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferEmptyFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferEmptyFlags2);
	form.description = "DDR Event Builder Buffer Empty Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferEmptyFlags2);
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatDDREventBuilderBufferHalfFullFlagsHigh()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderBufferHalfFullFlags2);
	form.description = "DDR Event Builder Buffer Half Full Flags (32-63)";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_EventBuilderBufferHalfFullFlags2);
	form.vals.push_back(o.str());
	return form;
}

// SERDES Serial Inversion Enable Register
bool DTCLib::DTC_Registers::ReadInvertSERDESRXInput(DTC_Link_ID link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDESTXRXInvertEnable);
	return data[link + 8];
}
void DTCLib::DTC_Registers::SetInvertSERDESRXInput(DTC_Link_ID link, bool invert)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDESTXRXInvertEnable);
	data[link + 8] = invert;
	WriteRegister_(data.to_ulong(), DTC_Register_SERDESTXRXInvertEnable);
}
bool DTCLib::DTC_Registers::ReadInvertSERDESTXOutput(DTC_Link_ID link)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDESTXRXInvertEnable);
	return data[link];
}
void DTCLib::DTC_Registers::SetInvertSERDESTXOutput(DTC_Link_ID link, bool invert)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SERDESTXRXInvertEnable);
	data[link] = invert;
	WriteRegister_(data.to_ulong(), DTC_Register_SERDESTXRXInvertEnable);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESSerialInversionEnable()
{
	auto form = CreateFormatter(DTC_Register_SERDESTXRXInvertEnable);
	form.description = "SERDES Serial Inversion Enable";
	form.vals.push_back("       ([Input, Output])");
	for (auto r : DTC_Links)
	{
		form.vals.push_back(std::string("Link ") + std::to_string(r) + ": [" + (ReadInvertSERDESRXInput(r) ? "x" : " ") +
							"," + (ReadInvertSERDESTXOutput(r) ? "x" : " ") + "]");
	}
	form.vals.push_back(std::string("CFO:    [") + (ReadInvertSERDESRXInput(DTC_Link_CFO) ? "x" : " ") + "," +
						(ReadInvertSERDESTXOutput(DTC_Link_CFO) ? "x" : " ") + "]");

	return form;
}

// Jitter Attenuator CSR Register
std::bitset<2> DTCLib::DTC_Registers::ReadJitterAttenuatorSelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_JitterAttenuatorCSR);
	std::bitset<2> output;
	output[0] = data[4];
	output[1] = data[5];
	return output;
}
void DTCLib::DTC_Registers::SetJitterAttenuatorSelect(std::bitset<2> data)
{
	std::bitset<32> regdata = ReadRegister_(DTC_Register_JitterAttenuatorCSR);
	regdata[4] = data[0];
	regdata[5] = data[1];
	WriteRegister_(regdata.to_ulong(), DTC_Register_JitterAttenuatorCSR);
}

bool DTCLib::DTC_Registers::ReadJitterAttenuatorReset()
{
	std::bitset<32> regdata = ReadRegister_(DTC_Register_JitterAttenuatorCSR);
	return regdata[0];
}
void DTCLib::DTC_Registers::ResetJitterAttenuator()
{
	std::bitset<32> regdata = ReadRegister_(DTC_Register_JitterAttenuatorCSR);
	regdata[0] = 1;
	WriteRegister_(regdata.to_ulong(), DTC_Register_JitterAttenuatorCSR);
	usleep(1000);
	regdata[0] = 0;
	WriteRegister_(regdata.to_ulong(), DTC_Register_JitterAttenuatorCSR);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatJitterAttenuatorCSR()
{
	auto form = CreateFormatter(DTC_Register_JitterAttenuatorCSR);
	form.description = "Jitter Attenuator CSR";
	form.vals.push_back(std::string("Select Low: [") + (ReadJitterAttenuatorSelect()[0] ? "x" : " ") + "]");
	form.vals.push_back(std::string("Select High: [") + (ReadJitterAttenuatorSelect()[1] ? "x" : " ") + "]");
	form.vals.push_back(std::string("Reset:   [") + (ReadJitterAttenuatorReset() ? "x" : " ") + "]");
	return form;
}

// EVB SERDES PRBS Register
bool DTCLib::DTC_Registers::ReadEVBSERDESPRBSErrorFlag()
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	return regVal[30];
}

uint8_t DTCLib::DTC_Registers::ReadEVBSERDESTXPRBSSEL()
{
	uint8_t regVal = static_cast<uint8_t>((ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus) & 0x7000) >> 12);
	return regVal;
}

void DTCLib::DTC_Registers::SetEVBSERDESTXPRBSSEL(uint8_t byte)
{
	uint8_t regVal = (ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus) & 0xFFFF8FFF);
	regVal += ((byte & 0x7) << 12);
	WriteRegister_(regVal, DTC_Register_EVBSERDESPRBSControlStatus);
}

uint8_t DTCLib::DTC_Registers::ReadEVBSERDESRXPRBSSEL()
{
	uint8_t regVal = static_cast<uint8_t>((ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus) & 0x700) >> 8);
	return regVal;
}

void DTCLib::DTC_Registers::SetEVBSERDESRXPRBSSEL(uint8_t byte)
{
	uint8_t regVal = (ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus) & 0xFFFFF8FF);
	regVal += ((byte & 0x7) << 8);
	WriteRegister_(regVal, DTC_Register_EVBSERDESPRBSControlStatus);
}

bool DTCLib::DTC_Registers::ReadEVBSERDESPRBSForceError()
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	return regVal[1];
}

void DTCLib::DTC_Registers::SetEVBSERDESPRBSForceError(bool flag)
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	regVal[1] = flag;
	WriteRegister_(regVal.to_ulong(), DTC_Register_EVBSERDESPRBSControlStatus);
}

void DTCLib::DTC_Registers::ToggleEVBSERDESPRBSForceError()
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	regVal.flip(0);
	WriteRegister_(regVal.to_ulong(), DTC_Register_EVBSERDESPRBSControlStatus);
}

bool DTCLib::DTC_Registers::ReadEVBSERDESPRBSReset()
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	return regVal[0];
}

void DTCLib::DTC_Registers::SetEVBSERDESPRBSReset(bool flag)
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	regVal[0] = flag;
	WriteRegister_(regVal.to_ulong(), DTC_Register_EVBSERDESPRBSControlStatus);
}

void DTCLib::DTC_Registers::ToggleEVBSERDESPRBSReset()
{
	std::bitset<32> regVal(ReadRegister_(DTC_Register_EVBSERDESPRBSControlStatus));
	regVal.flip(0);
	WriteRegister_(regVal.to_ulong(), DTC_Register_EVBSERDESPRBSControlStatus);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatEVBSERDESPRBSControl()
{
	auto form = CreateFormatter(DTC_Register_EVBSERDESPRBSControlStatus);
	form.description = "EVB SERDES PRBS Control / Status";
	form.vals.push_back(std::string("PRBS Error: [") + (ReadEVBSERDESPRBSErrorFlag() ? "x" : " ") + "]");
	std::stringstream o;
	o << "EVB SERDES TX PRBS Select: 0x" << std::hex << static_cast<int>(ReadEVBSERDESTXPRBSSEL());
	form.vals.push_back(o.str());
	o.str("");
	o.clear();
	o << "EVB SERDES RX PRBS Select: 0x" << std::hex << static_cast<int>(ReadEVBSERDESRXPRBSSEL());
	form.vals.push_back(std::string("PRBS Force Error: [") + (ReadEVBSERDESPRBSForceError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("PRBS Reset: [") + (ReadEVBSERDESPRBSReset() ? "x" : " ") + "]");

	return form;
}

// Missed CFO Packet Count Registers
uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing0()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing0);
}

uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing1()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing1);
}

uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing2()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing2);
}

uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing3()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing3);
}

uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing4()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing4);
}

uint32_t DTCLib::DTC_Registers::ReadMissedCFOPacketCountRing5()
{
	return ReadRegister_(DTC_Register_MissedCFOPacketCountRing5);
}

void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing0()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing0);
}
void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing1()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing1);
}
void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing2()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing2);
}
void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing3()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing3);
}
void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing4()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing4);
}
void DTCLib::DTC_Registers::ClearMissedCFOPacketCountRing5()
{
	WriteRegister_(1, DTC_Register_MissedCFOPacketCountRing5);
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing0()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing0);
	form.description = "Missed CFO Packet Count Link 0";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing0();
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing1()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing1);
	form.description = "Missed CFO Packet Count Link 1";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing1();
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing2()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing2);
	form.description = "Missed CFO Packet Count Link 2";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing2();
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing3()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing3);
	form.description = "Missed CFO Packet Count Link 3";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing3();
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing4()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing4);
	form.description = "Missed CFO Packet Count Link 4";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing4();
	form.vals.push_back(o.str());
	return form;
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatMissedCFOPacketCountRing5()
{
	auto form = CreateFormatter(DTC_Register_MissedCFOPacketCountRing5);
	form.description = "Missed CFO Packet Count Link 5";
	std::stringstream o;
	o << std::dec << ReadMissedCFOPacketCountRing5();
	form.vals.push_back(o.str());
	return form;
}

// Local Fragment Drop Count Register
uint32_t DTCLib::DTC_Registers::ReadLocalFragmentDropCount()
{
	return ReadRegister_(DTC_Register_LocalFragmentDropCount);
}

void DTCLib::DTC_Registers::ClearLocalFragmentDropCount() { WriteRegister_(1, DTC_Register_LocalFragmentDropCount); }

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatLocalFragmentDropCount()
{
	auto form = CreateFormatter(DTC_Register_LocalFragmentDropCount);
	form.description = "Local Data Fragment Drop Count";
	std::stringstream o;
	o << std::dec << ReadLocalFragmentDropCount();
	form.vals.push_back(o.str());
	return form;
}

// Event Builder Error Register
bool DTCLib::DTC_Registers::ReadEventBuilder_SubEventReceiverFlagsBufferError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[24];
}
bool DTCLib::DTC_Registers::ReadEventBuilder_EthernetInputFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[16];
}
bool DTCLib::DTC_Registers::ReadEventBuilder_LinkError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[9];
}
bool DTCLib::DTC_Registers::ReadEventBuilder_TXPacketError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[8];
}
bool DTCLib::DTC_Registers::ReadEventBuilder_LocalDataPointerFIFOQueueError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[1];
}
bool DTCLib::DTC_Registers::ReadEventBuilder_TransmitDMAByteCountFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_EventBuilderErrorFlags);
	return data[0];
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatEventBuilderErrorRegister()
{
	auto form = CreateFormatter(DTC_Register_EventBuilderErrorFlags);
	form.description = "Event Builder Error Flags";
	form.vals.push_back(std::string("Sub-Event Received Flags Buffer Error: [") +
						(ReadEventBuilder_SubEventReceiverFlagsBufferError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Input FIFO Full:                       [") +
						(ReadEventBuilder_EthernetInputFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Link Error:                            [") +
						(ReadEventBuilder_LinkError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX Packet Error:                       [") +
						(ReadEventBuilder_TXPacketError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Local Data Pointer FIFO Queue Error:   [") +
						(ReadEventBuilder_LocalDataPointerFIFOQueueError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Transmit DMA Byte Count FIFO Full:     [") +
						(ReadEventBuilder_TransmitDMAByteCountFIFOFull() ? "x" : " ") + "]");
	return form;
}

// SERDES VFIFO Error Register
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_EgressFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[10];
}
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_IngressFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[9];
}
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_EventByteCountTotalError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[8];
}
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_LastWordWrittenTimeoutError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[2];
}
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_FragmentCountError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[1];
}
bool DTCLib::DTC_Registers::ReadSERDESVFIFO_DDRFullError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_InputBufferErrorFlags);
	return data[0];
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSERDESVFIFOError()
{
	auto form = CreateFormatter(DTC_Register_InputBufferErrorFlags);
	form.description = "SERDES VFIFO Error Flags";
	form.vals.push_back(std::string("Egress FIFO Full:             [") + (ReadSERDESVFIFO_EgressFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("Ingress FIFO Full:            [") + (ReadSERDESVFIFO_IngressFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("Event Byte Count Total Error: [") +
						(ReadSERDESVFIFO_EventByteCountTotalError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Last Word Written Timeout:    [") +
						(ReadSERDESVFIFO_LastWordWrittenTimeoutError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Fragment count Error:         [") +
						(ReadSERDESVFIFO_FragmentCountError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("DDR Full Error:               [") + (ReadSERDESVFIFO_DDRFullError() ? "x" : " ") +
						"]");
	return form;
}

// PCI VFIFO Error Register
bool DTCLib::DTC_Registers::ReadPCIVFIFO_DDRFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[12];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_MemoryMappedWriteCompleteFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[11];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_PCIWriteEventFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[10];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_LocalDataPointerFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[9];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_EgressFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[8];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_RXBufferSelectFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[2];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_IngressFIFOFull()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[1];
}
bool DTCLib::DTC_Registers::ReadPCIVFIFO_EventByteCountTotalError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_OutputBufferErrorFlags);
	return data[0];
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatPCIVFIFOError()
{
	auto form = CreateFormatter(DTC_Register_OutputBufferErrorFlags);
	form.description = "PCI VFIFO Error Flags";
	form.vals.push_back(std::string("DDR Full Error:               [") + (ReadPCIVFIFO_DDRFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Memmap Write Cmplt FIFO Full: [") +
						(ReadPCIVFIFO_MemoryMappedWriteCompleteFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("PCI Write Event FIFO Full:    [") +
						(ReadPCIVFIFO_PCIWriteEventFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Local Data Pointer FIFO Full: [") +
						(ReadPCIVFIFO_LocalDataPointerFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Egress FIFO Full:             [") + (ReadPCIVFIFO_EgressFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("RX Buffer Select FIFO Full:   [") +
						(ReadPCIVFIFO_RXBufferSelectFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Ingress FIFO Ful:             [") + (ReadPCIVFIFO_IngressFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("Event Byte Count Total Error: [") +
						(ReadPCIVFIFO_EventByteCountTotalError() ? "x" : " ") + "]");
	return form;
}

// ROC Link Error Registers
bool DTCLib::DTC_Registers::ReadROCLink_ROCDataRequestSyncError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[5];
}
bool DTCLib::DTC_Registers::ReadROCLink_RXPacketCountError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[4];
}
bool DTCLib::DTC_Registers::ReadROCLink_RXPacketError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[3];
}
bool DTCLib::DTC_Registers::ReadROCLink_RXPacketCRCError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[2];
}
bool DTCLib::DTC_Registers::ReadROCLink_DataPendingTimeoutError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[1];
}
bool DTCLib::DTC_Registers::ReadROCLink_ReceiveDataPacketCountError(DTC_Link_ID link)
{
	std::bitset<32> data;
	switch (link)
	{
		case DTC_Link_0:
			data = ReadRegister_(DTC_Register_Link0ErrorFlags);
			break;
		case DTC_Link_1:
			data = ReadRegister_(DTC_Register_Link1ErrorFlags);
			break;
		case DTC_Link_2:
			data = ReadRegister_(DTC_Register_Link2ErrorFlags);
			break;
		case DTC_Link_3:
			data = ReadRegister_(DTC_Register_Link3ErrorFlags);
			break;
		case DTC_Link_4:
			data = ReadRegister_(DTC_Register_Link4ErrorFlags);
			break;
		case DTC_Link_5:
			data = ReadRegister_(DTC_Register_Link5ErrorFlags);
			break;
		default:
			return false;
	}
	return data[0];
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink0Error()
{
	auto form = CreateFormatter(DTC_Register_Link0ErrorFlags);
	form.description = "ROC Link 0 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_0) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_0) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_0) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_0) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_0) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_0) ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink1Error()
{
	auto form = CreateFormatter(DTC_Register_Link1ErrorFlags);
	form.description = "ROC Link 1 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_1) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_1) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_1) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_1) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_1) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_1) ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink2Error()
{
	auto form = CreateFormatter(DTC_Register_Link2ErrorFlags);
	form.description = "ROC Link 2 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_2) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_2) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_2) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_2) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_2) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_2) ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink3Error()
{
	auto form = CreateFormatter(DTC_Register_Link3ErrorFlags);
	form.description = "ROC Link 3 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_3) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_3) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_3) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_3) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_3) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_3) ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink4Error()
{
	auto form = CreateFormatter(DTC_Register_Link4ErrorFlags);
	form.description = "ROC Link 4 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_4) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_4) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_4) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_4) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_4) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_4) ? "x" : " ") + "]");
	return form;
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatRocLink5Error()
{
	auto form = CreateFormatter(DTC_Register_Link5ErrorFlags);
	form.description = "ROC Link 5 Error";
	form.vals.push_back(std::string("ROC Data Request Sync Error:     [") +
						(ReadROCLink_ROCDataRequestSyncError(DTC_Link_5) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Count Error:           [") +
						(ReadROCLink_RXPacketCountError(DTC_Link_5) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet Error:                 [") +
						(ReadROCLink_RXPacketError(DTC_Link_5) ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX Packet CRC Error:             [") +
						(ReadROCLink_RXPacketCRCError(DTC_Link_5) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Pending Timeout Error:      [") +
						(ReadROCLink_DataPendingTimeoutError(DTC_Link_5) ? "x" : " ") + "]");
	form.vals.push_back(std::string("Receive Data Packet Count Error: [") +
						(ReadROCLink_ReceiveDataPacketCountError(DTC_Link_5) ? "x" : " ") + "]");
	return form;
}

// CFO Link Error Register
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatCFOLinkError()
{
	auto form = CreateFormatter(DTC_Register_CFOLinkErrorFlags);
	form.description = "CFO Link Error Flags";
	std::stringstream o;
	o << "0x" << std::hex << ReadRegister_(DTC_Register_CFOLinkErrorFlags);
	form.vals.push_back(o.str());
	return form;
}

// Link Mux Error Register
bool DTCLib::DTC_Registers::ReadDCSMuxDecodeError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_LinkMuxErrorFlags);
	return data[1];
}
bool DTCLib::DTC_Registers::ReadDataMuxDecodeError()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_LinkMuxErrorFlags);
	return data[0];
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatLinkMuxError()
{
	auto form = CreateFormatter(DTC_Register_LinkMuxErrorFlags);
	form.description = "Link Mux Error Flags";
	form.vals.push_back(std::string("DCS Mux Decode Error:  [") + (ReadDCSMuxDecodeError() ? "x" : " ") + "]");
	form.vals.push_back(std::string("Data Mux Decode Error: [") + (ReadDataMuxDecodeError() ? "x" : " ") + "]");
	return form;
}

// Firefly CSR Register
bool DTCLib::DTC_Registers::ReadTXRXFireflyPresent()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[26];
}
bool DTCLib::DTC_Registers::ReadRXFireflyPresent()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[25];
}
bool DTCLib::DTC_Registers::ReadTXFireflyPresent()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[24];
}
bool DTCLib::DTC_Registers::ReadTXRXFireflyInterrupt()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[18];
}
bool DTCLib::DTC_Registers::ReadRXFireflyInterrupt()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[17];
}
bool DTCLib::DTC_Registers::ReadTXFireflyInterrupt()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[16];
}
bool DTCLib::DTC_Registers::ReadTXRXFireflySelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[10];
}
void DTCLib::DTC_Registers::SetTXRXFireflySelect(bool select)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[10] = select;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
bool DTCLib::DTC_Registers::ReadTXFireflySelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[9];
}
void DTCLib::DTC_Registers::SetTXFireflySelect(bool select)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[9] = select;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
bool DTCLib::DTC_Registers::ReadRXFireflySelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[8];
}
void DTCLib::DTC_Registers::SetRXFireflySelect(bool select)
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[8] = select;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
bool DTCLib::DTC_Registers::ReadResetTXRXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[2];
}
void DTCLib::DTC_Registers::ResetTXRXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[2] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
	usleep(1000);
	data[2] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
bool DTCLib::DTC_Registers::ReadResetTXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[1];
}
void DTCLib::DTC_Registers::ResetTXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
	usleep(1000);
	data[1] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
bool DTCLib::DTC_Registers::ReadResetRXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	return data[0];
}
void DTCLib::DTC_Registers::ResetRXFirefly()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_FireFlyControlStatus);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
	usleep(1000);
	data[0] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_FireFlyControlStatus);
}
DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFireflyCSR()
{
	auto form = CreateFormatter(DTC_Register_FireFlyControlStatus);
	form.description = "FireFly Control and Status";
	form.vals.push_back(std::string("TXRX FireFly Present:   [") + (ReadTXRXFireflyPresent() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX FireFly Present:     [") + (ReadRXFireflyPresent() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX FireFly Present:     [") + (ReadTXFireflyPresent() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TXRX FireFly Interrupt: [") + (ReadTXRXFireflyInterrupt() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX FireFly Interrupt:   [") + (ReadRXFireflyInterrupt() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX FireFly Interrupt:   [") + (ReadTXFireflyInterrupt() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TXRX FireFly Select:    [") + (ReadTXRXFireflySelect() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX FireFly Select:      [") + (ReadTXFireflySelect() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX FireFly Select:      [") + (ReadRXFireflySelect() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TXRX FireFly Reset:     [") + (ReadResetTXRXFirefly() ? "x" : " ") + "]");
	form.vals.push_back(std::string("TX FireFly Reset:       [") + (ReadResetTXFirefly() ? "x" : " ") + "]");
	form.vals.push_back(std::string("RX FireFly Reset:       [") + (ReadResetRXFirefly() ? "x" : " ") + "]");
	return form;
}

// SFP Control/Status Register
bool DTCLib::DTC_Registers::ReadSFPPresent()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	return data[31];
}

bool DTCLib::DTC_Registers::ReadSFPLOS()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	return data[17];
}

bool DTCLib::DTC_Registers::ReadSFPTXFault()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	return data[16];
}

void DTCLib::DTC_Registers::EnableSFPRateSelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	data[1] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_SFPControlStatus);
}

void DTCLib::DTC_Registers::DisableSFPRateSelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	data[1] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_SFPControlStatus);
}

bool DTCLib::DTC_Registers::ReadSFPRateSelect()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	return data[1];
}

void DTCLib::DTC_Registers::DisableSFPTX()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	data[0] = 1;
	WriteRegister_(data.to_ulong(), DTC_Register_SFPControlStatus);
}

void DTCLib::DTC_Registers::EnableSFPTX()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	data[0] = 0;
	WriteRegister_(data.to_ulong(), DTC_Register_SFPControlStatus);
}

bool DTCLib::DTC_Registers::ReadSFPTXDisable()
{
	std::bitset<32> data = ReadRegister_(DTC_Register_SFPControlStatus);
	return data[0];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatSFPControlStatus()
{
	auto form = CreateFormatter(DTC_Register_SFPControlStatus);
	form.description = "SFP Control Status";
	form.vals.push_back(std::string("SFP Present:     [") + (ReadSFPPresent() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SFP LOS:         [") + (ReadSFPLOS() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SFP TX Fault:    [") + (ReadSFPTXFault() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SFP Rate Select: [") + (ReadSFPRateSelect() ? "x" : " ") + "]");
	form.vals.push_back(std::string("SFP TX Disable:  [") + (ReadSFPTXDisable() ? "x" : " ") + "]");
	return form;
}

// FPGA PROM Program Status Register
bool DTCLib::DTC_Registers::ReadFPGAPROMProgramFIFOFull()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_FPGAPROMProgramStatus);
	return dataSet[1];
}

bool DTCLib::DTC_Registers::ReadFPGAPROMReady()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_FPGAPROMProgramStatus);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGAPROMProgramStatus()
{
	auto form = CreateFormatter(DTC_Register_FPGAPROMProgramStatus);
	form.description = "FPGA PROM Program Status";
	form.vals.push_back(std::string("FPGA PROM Program FIFO Full: [") + (ReadFPGAPROMProgramFIFOFull() ? "x" : " ") +
						"]");
	form.vals.push_back(std::string("FPGA PROM Ready:             [") + (ReadFPGAPROMReady() ? "x" : " ") + "]");
	return form;
}

// FPGA Core Access Register
void DTCLib::DTC_Registers::ReloadFPGAFirmware()
{
	WriteRegister_(0xFFFFFFFF, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0xAA995566, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x20000000, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x30020001, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x00000000, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x30008001, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x0000000F, DTC_Register_FPGACoreAccess);
	while (ReadFPGACoreAccessFIFOFull())
	{
		usleep(10);
	}
	WriteRegister_(0x20000000, DTC_Register_FPGACoreAccess);
}

bool DTCLib::DTC_Registers::ReadFPGACoreAccessFIFOFull()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_FPGACoreAccess);
	return dataSet[1];
}

bool DTCLib::DTC_Registers::ReadFPGACoreAccessFIFOEmpty()
{
	std::bitset<32> dataSet = ReadRegister_(DTC_Register_FPGACoreAccess);
	return dataSet[0];
}

DTCLib::DTC_RegisterFormatter DTCLib::DTC_Registers::FormatFPGACoreAccess()
{
	auto form = CreateFormatter(DTC_Register_FPGACoreAccess);
	form.description = "FPGA Core Access";
	form.vals.push_back(std::string("FPGA Core Access FIFO Full:  [") + (ReadFPGACoreAccessFIFOFull() ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA Core Access FIFO Empty: [") + (ReadFPGACoreAccessFIFOEmpty() ? "x" : " ") +
						"]");

	return form;
}

// Event Mode Lookup Table
void DTCLib::DTC_Registers::SetAllEventModeWords(uint32_t data)
{
	for (uint16_t address = DTC_Register_EventModeLookupTableStart; address <= DTC_Register_EventModeLookupTableEnd;
		 address += 4)
	{
		auto retry = 3;
		int errorCode;
		do
		{
			errorCode = device_.write_register(address, 100, data);
			--retry;
		} while (retry > 0 && errorCode != 0);
		if (errorCode != 0)
		{
			TLOG(TLVL_ERROR) << "Error writing register " << address;
			throw DTC_IOErrorException(errorCode);
		}
	}
}

void DTCLib::DTC_Registers::SetEventModeWord(uint8_t which, uint32_t data)
{
	uint16_t address = DTC_Register_EventModeLookupTableStart + (which * 4);
	if (address <= DTC_Register_EventModeLookupTableEnd)
	{
		auto retry = 3;
		int errorCode;
		do
		{
			errorCode = device_.write_register(address, 100, data);
			--retry;
		} while (retry > 0 && errorCode != 0);
		if (errorCode != 0)
		{
			TLOG(TLVL_ERROR) << "Error writing register " << address;
			throw DTC_IOErrorException(errorCode);
		}
	}
}

uint32_t DTCLib::DTC_Registers::ReadEventModeWord(uint8_t which)
{
	uint16_t address = DTC_Register_EventModeLookupTableStart + (which * 4);
	if (address <= DTC_Register_EventModeLookupTableEnd)
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
			TLOG(TLVL_ERROR) << "Error writing register " << address;
			throw DTC_IOErrorException(errorCode);
		}

		return data;
	}
	return 0;
}

// Oscillator Programming (DDR and SERDES)
bool DTCLib::DTC_Registers::SetNewOscillatorFrequency(DTC_OscillatorType oscillator, double targetFrequency)
{
	auto currentFrequency = ReadCurrentFrequency(oscillator);
	auto currentProgram = ReadCurrentProgram(oscillator);
	DTC_TLOG(TLVL_DEBUG) << "Target Frequency: " << targetFrequency << ", Current Frequency: " << currentFrequency
						 << ", Current Program: " << std::showbase << std::hex << currentProgram;

	// Check if targetFrequency is essentially the same as the current frequency...
	if (fabs(currentFrequency - targetFrequency) < targetFrequency * 30 / 1000000)
	{
		DTC_TLOG(TLVL_INFO) << "New frequency and old frequency are within 30 ppm of each other, not reprogramming!";
		return false;
	}

	auto newParameters = CalculateFrequencyForProgramming_(targetFrequency, currentFrequency, currentProgram);
	if (newParameters == 0)
	{
		DTC_TLOG(TLVL_WARNING) << "New program calculated as 0! Check parameters!";
		return false;
	}
	WriteCurrentProgram(newParameters, oscillator);
	WriteCurrentFrequency(targetFrequency, oscillator);
	return true;
}

double DTCLib::DTC_Registers::ReadCurrentFrequency(DTC_OscillatorType oscillator)
{
	switch (oscillator)
	{
		case DTC_OscillatorType_SERDES:
			return ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_EVB);
		case DTC_OscillatorType_Timing:
			return ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_CFO);
		case DTC_OscillatorType_DDR:
			return ReadDDROscillatorReferenceFrequency();
	}
	return 0;
}

uint64_t DTCLib::DTC_Registers::ReadCurrentProgram(DTC_OscillatorType oscillator)
{
	switch (oscillator)
	{
		case DTC_OscillatorType_SERDES:
			return ReadSERDESOscillatorParameters_();
		case DTC_OscillatorType_Timing:
			return ReadTimingOscillatorParameters_();
		case DTC_OscillatorType_DDR:
			return ReadDDROscillatorParameters_();
	}
	return 0;
}
void DTCLib::DTC_Registers::WriteCurrentFrequency(double freq, DTC_OscillatorType oscillator)
{
	auto newFreq = static_cast<uint32_t>(freq);
	switch (oscillator)
	{
		case DTC_OscillatorType_SERDES:
			SetSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_EVB, newFreq);
			break;
		case DTC_OscillatorType_Timing:
			SetSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress_CFO, newFreq);
			break;
		case DTC_OscillatorType_DDR:
			SetDDROscillatorReferenceFrequency(newFreq);
			break;
	}
}
void DTCLib::DTC_Registers::WriteCurrentProgram(uint64_t program, DTC_OscillatorType oscillator)
{
	switch (oscillator)
	{
		case DTC_OscillatorType_SERDES:
			SetSERDESOscillatorParameters_(program);
			break;
		case DTC_OscillatorType_Timing:
			SetTimingOscillatorParameters_(program);
			break;
		case DTC_OscillatorType_DDR:
			SetDDROscillatorParameters_(program);
			break;
	}
}

// Private Functions
void DTCLib::DTC_Registers::WriteRegister_(uint32_t data, const DTC_Register& address)
{
	auto retry = 3;
	int errorCode;
	do
	{
		errorCode = device_.write_register(address, 100, data);
		--retry;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0)
	{
		TLOG(TLVL_ERROR) << "Error writing register 0x" << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		throw DTC_IOErrorException(errorCode);
	}
}

uint32_t DTCLib::DTC_Registers::ReadRegister_(const DTC_Register& address)
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
		TLOG(TLVL_ERROR) << "Error reading register 0x" << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		throw DTC_IOErrorException(errorCode);
	}

	DTC_TLOG(21) << "ReadRegister_ returning " << std::hex << std::showbase << data << " for address " << static_cast<uint32_t>(address);
	return data;
}

int DTCLib::DTC_Registers::DecodeHighSpeedDivider_(int input)
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

int DTCLib::DTC_Registers::EncodeHighSpeedDivider_(int input)
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

int DTCLib::DTC_Registers::EncodeOutputDivider_(int input)
{
	if (input == 1) return 0;
	int temp = input / 2;
	return (temp * 2) - 1;
}

uint64_t DTCLib::DTC_Registers::CalculateFrequencyForProgramming_(double targetFrequency, double currentFrequency,
																  uint64_t currentProgram)
{
	DTC_TLOG(4) << "CalculateFrequencyForProgramming: targetFrequency=" << targetFrequency << ", currentFrequency=" << currentFrequency
				<< ", currentProgram=" << std::showbase << std::hex << static_cast<unsigned long long>(currentProgram);
	auto currentHighSpeedDivider = DecodeHighSpeedDivider_((currentProgram >> 45) & 0x7);
	auto currentOutputDivider = DecodeOutputDivider_((currentProgram >> 38) & 0x7F);
	auto currentRFREQ = DecodeRFREQ_(currentProgram & 0x3FFFFFFFFF);
	DTC_TLOG(4) << "CalculateFrequencyForProgramming: Current HSDIV=" << currentHighSpeedDivider << ", N1=" << currentOutputDivider << ", RFREQ=" << currentRFREQ;
	const auto minFreq = 4850000000;  // Hz
	const auto maxFreq = 5670000000;  // Hz

	auto fXTAL = currentFrequency * currentHighSpeedDivider * currentOutputDivider / currentRFREQ;
	DTC_TLOG(4) << "CalculateFrequencyForProgramming: fXTAL=" << fXTAL;

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
		DTC_TLOG(4) << "CalculateFrequencyForProgramming: Adding solution: HSDIV=" << hsdiv << ", N1=" << thisN << ", fdco_new=" << fdco_new;
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
	DTC_TLOG(4) << "CalculateFrequencyForProgramming: New Program: HSDIV=" << newHighSpeedDivider << ", N1=" << newOutputDivider << ", RFREQ=" << newRFREQ;

	if (EncodeHighSpeedDivider_(newHighSpeedDivider) == -1)
	{
		DTC_TLOG(0) << "ERROR: CalculateFrequencyForProgramming: Invalid HSDIV " << newHighSpeedDivider << "!";
		return 0;
	}
	if (newOutputDivider > 128 || newOutputDivider < 0)
	{
		DTC_TLOG(0) << "ERROR: CalculateFrequencyForProgramming: Invalid N1 " << newOutputDivider << "!";
		return 0;
	}
	if (newRFREQ <= 0)
	{
		DTC_TLOG(0) << "ERROR: CalculateFrequencyForProgramming: Invalid RFREQ " << newRFREQ << "!";
		return 0;
	}

	auto output = (static_cast<uint64_t>(EncodeHighSpeedDivider_(newHighSpeedDivider)) << 45) +
				  (static_cast<uint64_t>(EncodeOutputDivider_(newOutputDivider)) << 38) + EncodeRFREQ_(newRFREQ);
	DTC_TLOG(4) << "CalculateFrequencyForProgramming: New Program: " << std::showbase << std::hex << static_cast<unsigned long long>(output);
	return output;
}

uint64_t DTCLib::DTC_Registers::ReadSERDESOscillatorParameters_()
{
	uint64_t data = (static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 7)) << 40) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 8)) << 32) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 9)) << 24) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 10)) << 16) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 11)) << 8) +
					static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_EVB, 12));
	return data;
}

uint64_t DTCLib::DTC_Registers::ReadTimingOscillatorParameters_()
{
	uint64_t data = (static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 7)) << 40) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 8)) << 32) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 9)) << 24) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 10)) << 16) +
					(static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 11)) << 8) +
					static_cast<uint64_t>(ReadSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 12));
	return data;
}

uint64_t DTCLib::DTC_Registers::ReadDDROscillatorParameters_()
{
	uint64_t data = (static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 7)) << 40) +
					(static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 8)) << 32) +
					(static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 9)) << 24) +
					(static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 10)) << 16) +
					(static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 11)) << 8) +
					static_cast<uint64_t>(ReadDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 12));
	return data;
}

void DTCLib::DTC_Registers::SetSERDESOscillatorParameters_(uint64_t program)
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

void DTCLib::DTC_Registers::SetTimingOscillatorParameters_(uint64_t program)
{
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 0x89, 0x10);

	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 7, static_cast<uint8_t>(program >> 40));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 8, static_cast<uint8_t>(program >> 32));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 9, static_cast<uint8_t>(program >> 24));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 10, static_cast<uint8_t>(program >> 16));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 11, static_cast<uint8_t>(program >> 8));
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 12, static_cast<uint8_t>(program));

	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 0x89, 0);
	WriteSERDESIICInterface(DTC_IICSERDESBusAddress_CFO, 0x87, 0x40);
}

void DTCLib::DTC_Registers::SetDDROscillatorParameters_(uint64_t program)
{
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 0x89, 0x10);

	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 7, static_cast<uint8_t>(program >> 40));
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 8, static_cast<uint8_t>(program >> 32));
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 9, static_cast<uint8_t>(program >> 24));
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 10, static_cast<uint8_t>(program >> 16));
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 11, static_cast<uint8_t>(program >> 8));
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 12, static_cast<uint8_t>(program));

	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 0x89, 0);
	WriteDDRIICInterface(DTC_IICDDRBusAddress_DDROscillator, 0x87, 0x40);
}

bool DTCLib::DTC_Registers::WaitForLinkReady_(DTC_Link_ID const& link, size_t interval, double timeout /*seconds*/)
{
	auto start = std::chrono::steady_clock::now();
	auto last_print = start;
	bool ready = ReadSERDESPLLLocked(link) && ReadResetRXSERDESDone(link) && ReadResetTXSERDESDone(link) && ReadSERDESRXCDRLock(link);

	while (!ready)
	{
		usleep(interval);
		ready = ReadSERDESPLLLocked(link) && ReadResetRXSERDESDone(link) && ReadResetTXSERDESDone(link) && ReadSERDESRXCDRLock(link);
		if (std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::steady_clock::now() - last_print).count() > 5.0)
		{
			DTC_TLOG(TLVL_DEBUG) << "DTC_Registers: WaitForLinkReady_: ROC Link " << link << ": PLL Locked: " << std::boolalpha << ReadSERDESPLLLocked(link) << ", RX Reset Done: " << ReadResetRXSERDESDone(link) << ", TX Reset Done: " << ReadResetTXSERDESDone(link) << ", CDR Lock: " << ReadSERDESRXCDRLock(link);
			last_print = std::chrono::steady_clock::now();
		}
		if (std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::steady_clock::now() - start).count() > timeout)
		{
			DTC_TLOG(TLVL_ERROR) << "DTC_Registers: WaitForLinkReady_ ABORTING: ROC Link " << link << ": PLL Locked: " << std::boolalpha << ReadSERDESPLLLocked(link) << ", RX Reset Done: " << ReadResetRXSERDESDone(link) << ", TX Reset Done: " << ReadResetTXSERDESDone(link) << ", CDR Lock: " << ReadSERDESRXCDRLock(link);
			return false;
		}
	}
	return true;
}
