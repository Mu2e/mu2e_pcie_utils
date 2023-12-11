#include "CFOandDTC_Registers.h"

#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <cmath>
#include <iomanip>  // std::setw, std::setfill
#include <sstream>  // Convert uint to hex string

#include "TRACE/tracemf.h"

#include "dtcInterfaceLib/otsStyleCoutMacros.h"


#undef 	__COUT_HDR__ 
#define __COUT_HDR__ 		"CFOandDTC " << device_.getDeviceUID() << ": "
// #define CFO_DTC_TLOG(lvl) TLOG(lvl) << "CFO-DTC " << device_.getDeviceUID() << ": "
#define TLVL_ResetDTC TLVL_DEBUG + 5
#define TLVL_AutogenDRP TLVL_DEBUG + 6
#define TLVL_SERDESReset TLVL_DEBUG + 7
#define TLVL_CalculateFreq TLVL_DEBUG + 8
#define TLVL_ReadRegister TLVL_DEBUG + 20


using namespace DTCLib;

// struct CFOandDTC_Register { 
enum CFOandDTC_Register : uint16_t
{
	DTCLIB_COMMON_REGISTERS
};

// }; //end CFOandDTC_Register enum


/// <summary>
/// Construct an instance of the core CFO-and-DTC register map
/// </summary>
DTCLib::CFOandDTC_Registers::CFOandDTC_Registers()
	: device_()
{
} //end constructor()

/// <summary>
/// CFOandDTC_Registers destructor
/// </summary>
DTCLib::CFOandDTC_Registers::~CFOandDTC_Registers()
{
	TLOG(TLVL_INFO) << "DESTRUCTOR";
	device_.close();
} //end destructor()

/// <summary>
/// Perform a register dump
/// </summary>
/// <param name="width">Printable width of description fields</param>
/// <returns>String containing all registers, with their human-readable representations</returns>
std::string DTCLib::CFOandDTC_Registers::FormattedRegDump(int width,
	const std::vector<std::function<RegisterFormatter()>>& regVec)
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
	for (auto i : regVec)
	{
		o << divider << std::endl;
		o << i();
	}
	return o.str();
} //end FormattedRegDump()

// Desgin Version/Date Registers
/// <summary>
/// Read the design version
/// </summary>
/// <returns>Design version, in VersionNumber_Date format</returns>
std::string DTCLib::CFOandDTC_Registers::ReadDesignVersion() { return //ReadDesignVersionNumber() + "_" + 
	ReadDesignDate() + "_" + ReadVivadoVersionNumber() + "_" + ReadDesignLinkSpeed() + "_" + ReadDesignType(); }

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatDesignVersion()
{
	auto form = CreateFormatter(CFOandDTC_Register_DesignVersion);
	form.description = "DTC Firmware Design Version";
	form.vals.push_back(std::string("Version Number:                [") + ReadDesignVersionNumber());
	form.vals.push_back(std::string("Link Speed CFO link/ROC links: [") + ReadDesignLinkSpeed());
	form.vals.push_back(std::string("Design Type (C=CFO, D=DTC):    [") + ReadDesignType());
	return form;
}

/// <summary>
/// Read the modification date of the DTC firmware
/// </summary>
/// <returns>Design date in MON/DD/20YY HH:00 format</returns>
std::string DTCLib::CFOandDTC_Registers::ReadDesignDate()
{
	auto readData = ReadRegister_(CFOandDTC_Register_DesignDate);
	std::ostringstream o;
	std::vector<std::string> months({"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"});
	int mon =  ((readData>>20)&0xF)*10 + ((readData>>16)&0xF);
	if(mon-1 > 11)
	{ 
		__SS__ << "Invalid register read for firmware design date: " + std::to_string(mon) << 
			". If the value is 165, this likely means the PCIe in not initialized and perhaps a PCIe reset of the linux system would fix the issue.";				
		__SS_THROW__;
		// throw std::runtime_error("Invalid register read for firmware design date: " + std::to_string(mon));
	}
	o << months[mon-1] << "/" << 
		((readData>>12)&0xF) << ((readData>>8)&0xF) << "/20" << 
		((readData>>28)&0xF) << ((readData>>24)&0xF) << " " <<
		((readData>>4)&0xF) << ((readData>>0)&0xF) << ":00   raw-data: 0x" << std::hex << readData;
	return o.str();
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatDesignDate()
{
	auto form = CreateFormatter(CFOandDTC_Register_DesignDate);
	form.description = "DTC Firmware Design Date";
	form.vals.push_back(ReadDesignDate());
	return form;
}

/// <summary>
/// Read the design version number
/// </summary>
/// <returns>The design version number, in vMM.mm format</returns>
std::string DTCLib::CFOandDTC_Registers::ReadDesignVersionNumber()
{
	auto data = ReadRegister_(CFOandDTC_Register_DesignVersion);
	int minor = data & 0xFF;
	int major = (data & 0xFF00) >> 8;

	__COUT_INFO__ << "Driver Version: " << GetDevice()->get_driver_version();
	return "v" + std::to_string(major) + "." + std::to_string(minor);
} // end ReadDesignVersionNumber()

/// <summary>
/// Read the design link speed
/// </summary>
/// <returns>The design link speed, in hex character format</returns>
std::string DTCLib::CFOandDTC_Registers::ReadDesignLinkSpeed()
{
	auto data = ReadRegister_(CFOandDTC_Register_DesignVersion);
	int cfoLinkSpeed = (data>>28) & 0xF;	//bit28: 0x4 for 4G
	int rocLinkSpeed = (data>>24) & 0xF;	//bit24: 0x3 for 3.125G
	return std::to_string(cfoLinkSpeed) + ".0G/" +
		std::to_string(rocLinkSpeed) + (rocLinkSpeed==3?".125":".0") +
		"G";
} // end ReadDesignLinkSpeed()

/// <summary>
/// Read the design type
/// </summary>
/// <returns>The design link type, 0xC for CFO and 0xD for DTC </returns>
std::string DTCLib::CFOandDTC_Registers::ReadDesignType()
{
	auto data = ReadRegister_(CFOandDTC_Register_DesignVersion);
	int type = (data>>16) & 0xF;	//0xC for CFO and 0xD for DTC
	return (type==0xC?"C":(type==0xD?"D":"U")); //U for unknown!
} // end ReadDesignLinkSpeed()

/// <summary>
/// Read the FPGA On-die Temperature sensor
/// </summary>
/// <returns>Temperature of the FGPA (deg. C)</returns>
double DTCLib::CFOandDTC_Registers::ReadFPGATemperature()
{
	auto val = ReadRegister_(CFOandDTC_Register_FPGA_Temperature);

	return ((val * 503.975) / 4096.0) - 273.15;
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatFPGATemperature()
{
	auto form = CreateFormatter(CFOandDTC_Register_FPGA_Temperature);
	form.description = "FPGA Temperature";
	form.vals.push_back(std::to_string(ReadFPGATemperature()) + " C");
	return form;
}

/// <summary>
/// Read the FPGA VCC INT Voltage (Nominal is 1.0 V)
/// </summary>
/// <returns>FPGA VCC INT Voltage (V)</returns>
double DTCLib::CFOandDTC_Registers::ReadFPGAVCCINTVoltage()
{
	auto val = ReadRegister_(CFOandDTC_Register_FPGA_VCCINT);
	return (val / 4095.0) * 3.0;
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatFPGAVCCINT()
{
	auto form = CreateFormatter(CFOandDTC_Register_FPGA_VCCINT);
	form.description = "FPGA VCC INT";
	form.vals.push_back(std::to_string(ReadFPGAVCCINTVoltage()) + " V");
	return form;
}

/// <summary>
/// Read the FPGA VCC AUX Voltage (Nominal is 1.8 V)
/// </summary>
/// <returns>FPGA VCC AUX Voltage (V)</returns>
double DTCLib::CFOandDTC_Registers::ReadFPGAVCCAUXVoltage()
{
	auto val = ReadRegister_(CFOandDTC_Register_FPGA_VCCAUX);
	return (val / 4095.0) * 3.0;
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatFPGAVCCAUX()
{
	auto form = CreateFormatter(CFOandDTC_Register_FPGA_VCCAUX);
	form.description = "FPGA VCC AUX";
	form.vals.push_back(std::to_string(ReadFPGAVCCAUXVoltage()) + " V");
	return form;
}

/// <summary>
/// Read the FPGA VCC BRAM Voltage (Nominal 1.0 V)
/// </summary>
/// <returns>FPGA VCC BRAM Voltage (V)</returns>
double DTCLib::CFOandDTC_Registers::ReadFPGAVCCBRAMVoltage()
{
	auto val = ReadRegister_(CFOandDTC_Register_FPGA_VCCBRAM);
	return (val / 4095.0) * 3.0;
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatFPGAVCCBRAM()
{
	auto form = CreateFormatter(CFOandDTC_Register_FPGA_VCCBRAM);
	form.description = "FPGA VCC BRAM";
	form.vals.push_back(std::to_string(ReadFPGAVCCBRAMVoltage()) + " V");
	return form;
}

/// <summary>
/// Read the value of the FPGA Die Temperature Alarm bit
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadFPGADieTemperatureAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[8];
}

/// <summary>
/// Reset the FPGA Die Temperature Alarm bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetFPGADieTemperatureAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[8] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Read the FPGA Alarms bit (OR of VCC and User Temperature alarm bits)
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadFPGAAlarms(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[7];
}

/// <summary>
/// Reset the FPGA Alarms bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetFPGAAlarms(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[7] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Read the VCC BRAM Alarm bit
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadVCCBRAMAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[3];
}

/// <summary>
/// Reset the VCC BRAM Alarm bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetVCCBRAMAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[3] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Read the VCC AUX Alarm bit
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadVCCAUXAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[2];
}

/// <summary>
/// Reset the VCC AUX Alarm bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetVCCAUXAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[2] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Read the VCC INT Alarm bit
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadVCCINTAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[1];
}

/// <summary>
/// Reset the VCC INT Alarm bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetVCCINTAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[1] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Read the FPGA User Temperature Alarm bit
/// </summary>
/// <returns>Value of the bit</returns>
bool DTCLib::CFOandDTC_Registers::ReadFPGAUserTemperatureAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	return data[0];
}

/// <summary>
/// Reset the FPGA User Temperature Alarm bit
/// </summary>
void DTCLib::CFOandDTC_Registers::ResetFPGAUserTemperatureAlarm(std::optional<uint32_t> val)
{
	std::bitset<32> data = val.value_or(ReadRegister_(CFOandDTC_Register_FPGA_MonitorAlarm));
	data[0] = 1;
	WriteRegister_(data.to_ulong(), CFOandDTC_Register_FPGA_MonitorAlarm);
}

/// <summary>
/// Formats the register's current value for register dumps
/// </summary>
/// <returns>RegisterFormatter object containing register information</returns>
DTCLib::RegisterFormatter DTCLib::CFOandDTC_Registers::FormatFPGAAlarms()
{
	auto form = CreateFormatter(CFOandDTC_Register_FPGA_MonitorAlarm);
	form.description = "FPGA Monitor Alarm";
	form.vals.push_back("[ x = 1 (hi) ]"); //translation
	form.vals.push_back(std::string("FPGA Die Temperature Alarm:  [") + (ReadFPGADieTemperatureAlarm(form.value) ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA Alarms OR:              [") + (ReadFPGAAlarms(form.value) ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC BRAM Alarm:              [") + (ReadVCCBRAMAlarm(form.value) ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC AUX Alarm:               [") + (ReadVCCAUXAlarm(form.value) ? "x" : " ") + "]");
	form.vals.push_back(std::string("VCC INT Alarm:               [") + (ReadVCCINTAlarm(form.value) ? "x" : " ") + "]");
	form.vals.push_back(std::string("FPGA User Temperature Alarm: [") + (ReadFPGAUserTemperatureAlarm(form.value) ? "x" : " ") + "]");

	return form;
}


// Private Functions
uint32_t DTCLib::CFOandDTC_Registers::WriteRegister_(uint32_t dataToWrite, const CFOandDTC_Register& address)
{
	auto retry = 3;
	int errorCode;
	uint32_t readbackValue;
	do
	{
		errorCode = device_.write_register_checked(address, 100, dataToWrite, &readbackValue);
		--retry;
	} while (retry > 0 && errorCode != 0);
	if (errorCode != 0)
	{
		__SS__ << "Error writing register 0x" << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		// CFO_DTC_TLOG(TLVL_ERROR) << ss.str();		
		__SS_THROW__;
		// throw DTC_IOErrorException(errorCode);
	}

	{	//trace seems to ignore the std::setfill, so using stringstream
		std::stringstream o;
		o << "write value 0x"	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(dataToWrite)
				<< " to register 0x" 	<< std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << 
				std::endl;
		__COUT__ << o.str();
	}

	//verify register readback
	if(1)
	{
		uint32_t readbackValue = ReadRegister_(address);
		VerifyRegisterWrite_(address,readbackValue,dataToWrite); //virtual function call
		return readbackValue;
	} //end verify register readback
} //end WriteRegister_()

uint32_t DTCLib::CFOandDTC_Registers::ReadRegister_(const CFOandDTC_Register& address)
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
		__SS__ << "Error reading register 0x" << std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << " " << errorCode;
		__SS_THROW__;
		// throw DTC_IOErrorException(errorCode);
	}

	if(address != 0x916c)
	{	//trace seems to ignore the std::setfill, so using stringstream
		std::stringstream o;
		o << "read value 0x"	<< std::setw(8) << std::setfill('0') << std::setprecision(8) << std::hex << static_cast<uint32_t>(data)
			<< " from register 0x" 	<< std::setw(4) << std::setfill('0') << std::setprecision(4) << std::hex << static_cast<uint32_t>(address) << 
			std::endl;
		__COUT__ << o.str();
	}

	return data;
} //end ReadRegister_()

bool DTCLib::CFOandDTC_Registers::GetBit_(const CFOandDTC_Register& address, size_t bit)
{
	if (bit > 31)
	{
		__SS__ << "Cannot read bit " << bit << ", as it is out of range";
		__SS_THROW__;
		// throw std::out_of_range("Cannot read bit " + std::to_string(bit) + ", as it is out of range");
	}
	return std::bitset<32>(ReadRegister_(address))[bit];
}

void DTCLib::CFOandDTC_Registers::SetBit_(const CFOandDTC_Register& address, size_t bit, bool value)
{
	if (bit > 31)
	{
		__SS__ << "Cannot set bit " << bit << ", as it is out of range";
		__SS_THROW__;
		// throw std::out_of_range("Cannot set bit " + std::to_string(bit) + ", as it is out of range");
	}
	auto regVal = std::bitset<32>(ReadRegister_(address));
	regVal[bit] = value;
	WriteRegister_(regVal.to_ulong(), address);
}
