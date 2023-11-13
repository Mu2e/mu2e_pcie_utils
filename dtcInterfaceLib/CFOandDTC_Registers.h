#ifndef CFO_AND_DTC_REGISTERS_H
#define CFO_AND_DTC_REGISTERS_H

//#include <bitset> // std::bitset
//#include <cstdint> // uint8_t, uint16_t
#include <functional>  // std::bind, std::function
#include <vector>      // std::vector

#include "mu2edev.h"

namespace DTCLib {

	typedef uint16_t CFOandDTC_Register; //defined enum in CFOandDTC_Registers.cpp


/// <summary>
/// The RegisterFormatter class is used to print a DTC register in a human-readable format
/// </summary>
struct RegisterFormatter
{
	/// <summary>
	/// Default Constructor with zero values
	/// </summary>
	RegisterFormatter()
		: address(0), value(0), descWidth(28), description(""), vals() {}

	/// <summary>
	/// Default Copy Consturctor
	/// </summary>
	/// <param name="r">RegisterFormatter to copy</param>
	RegisterFormatter(const RegisterFormatter& r) = default;
	/// <summary>
	/// Default Move Constructor
	/// </summary>
	/// <param name="r">RegisterFormatter rvalue</param>
	RegisterFormatter(RegisterFormatter&& r) = default;
	uint16_t address;               ///< Address of the register
	uint32_t value;                 ///< Value of the register
	int descWidth;                  ///< Display width of description field
	std::string description;        ///< Description of the register (name)
	std::vector<std::string> vals;  ///< Human-readable descriptions of register values (bits, etc)

	/// <summary>
	/// Default Copy Assignment Operator
	/// </summary>
	/// <param name="r">RHS</param>
	/// <returns>RegisterFormatter regerence</returns>
	RegisterFormatter& operator=(const RegisterFormatter& r) = default;

	/// <summary>
	/// Write out the RegisterFormatter to the given stream. This function uses setw to make sure that fields for
	/// different registers still line up.
	/// Format is: "    0xADDR  | 0xVALUEXXX | DESCRIPTION: Variable size, minimum of 28 chars | Value 1
	///                                                                                        | Value 2 ..."
	/// </summary>
	/// <param name="stream">Stream to write register data to</param>
	/// <param name="reg">RegisterFormatter to write</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const RegisterFormatter& reg)
	{
		stream << std::hex << std::setfill('0');
		stream << "    0x" << std::setw(4) << static_cast<int>(reg.address) << "  | 0x" << std::setw(8)
			   << static_cast<int>(reg.value) << " | ";
		auto tmp = reg.description;
		tmp.resize(reg.descWidth, ' ');
		stream << tmp << " | ";

		if (!reg.vals.empty()) {
		auto first = true;
		for (auto i : reg.vals)
		{
			if (!first)
			{
				std::string placeholder = "";
				placeholder.resize(reg.descWidth, ' ');
				stream << "                           " << placeholder << " | ";
			}
			stream << i << std::endl;
			first = false;
		}
		}
		else
		{
			stream << std::endl;
		}

		return stream;
	}
}; // end RegisterFormatter class

/// <summary>
/// The CFOandDTC_Registers class represents the common CFO-and-DTC Register space, and all the methods necessary to read and write those
/// registers. Each register has, at the very least, a read method, a write method, and a RegisterFormatter method
/// which formats the register value in a human-readable way.
/// </summary>
class CFOandDTC_Registers
{
public:
	explicit CFOandDTC_Registers();

	virtual ~CFOandDTC_Registers();

	/// <summary>
	/// Get a pointer to the device handle
	/// </summary>
	/// <returns>mu2edev* pointer</returns>
	mu2edev* GetDevice() { return &device_; }


	//
	// Register IO Functions
	//

	// Desgin Version/Date Registers
	std::string ReadDesignVersion();
	RegisterFormatter FormatDesignVersion();
	std::string ReadDesignDate();
	RegisterFormatter FormatDesignDate();
	std::string ReadDesignVersionNumber();
	std::string ReadDesignLinkSpeed();
	std::string ReadDesignType();

	// Vivado Version Register
	virtual std::string ReadVivadoVersionNumber(uint32_t* val = 0) = 0;
	virtual RegisterFormatter FormatVivadoVersion() = 0;

	// FPGA Temperature Register
	double ReadFPGATemperature();
	RegisterFormatter FormatFPGATemperature();

	// FPGA VCCINT Voltage Register
	double ReadFPGAVCCINTVoltage();
	RegisterFormatter FormatFPGAVCCINT();

	// FPGA VCCAUX Voltage Register
	double ReadFPGAVCCAUXVoltage();
	RegisterFormatter FormatFPGAVCCAUX();

	// FPGA VCCBRAM Voltage Register
	double ReadFPGAVCCBRAMVoltage();
	RegisterFormatter FormatFPGAVCCBRAM();

	// FPGA Monitor Alarm Register
	bool ReadFPGADieTemperatureAlarm(std::optional<uint32_t> val);
	void ResetFPGADieTemperatureAlarm(std::optional<uint32_t> val);
	bool ReadFPGAAlarms(std::optional<uint32_t> val);
	void ResetFPGAAlarms(std::optional<uint32_t> val);
	bool ReadVCCBRAMAlarm(std::optional<uint32_t> val);
	void ResetVCCBRAMAlarm(std::optional<uint32_t> val);
	bool ReadVCCAUXAlarm(std::optional<uint32_t> val);
	void ResetVCCAUXAlarm(std::optional<uint32_t> val);
	bool ReadVCCINTAlarm(std::optional<uint32_t> val);
	void ResetVCCINTAlarm(std::optional<uint32_t> val);
	bool ReadFPGAUserTemperatureAlarm(std::optional<uint32_t> val);
	void ResetFPGAUserTemperatureAlarm(std::optional<uint32_t> val);
	RegisterFormatter FormatFPGAAlarms();
	
	std::string FormattedRegDump(int width, const std::vector<std::function<RegisterFormatter()>>& regVec);

protected:
	uint32_t WriteRegister_(uint32_t data, const CFOandDTC_Register& address);
	uint32_t ReadRegister_(const CFOandDTC_Register& address);
	virtual void VerifyRegisterWrite_(const CFOandDTC_Register& address, uint32_t readbackValue, uint32_t dataToWrite) = 0;

	/// <summary>
	/// Initializes a RegisterFormatter for the given CFOandDTC_Register
	/// </summary>
	/// <param name="address">Address of register to format</param>
	/// <returns>RegisterFormatter with address and raw value set</returns>
	RegisterFormatter CreateFormatter(const CFOandDTC_Register& address, bool readValue = true)
	{
		RegisterFormatter form;
		form.descWidth = formatterWidth_;
		form.address = address;
		if(readValue)
			form.value = ReadRegister_(address);
		return form;
	}

	
	bool GetBit_(const CFOandDTC_Register& address, size_t bit);
	void SetBit_(const CFOandDTC_Register& address, size_t bit, bool value);
	bool ToggleBit_(const CFOandDTC_Register& address, size_t bit)
	{
		auto val = GetBit_(address, bit);
		SetBit_(address, bit, !val);
		return !val;
	}

	mu2edev device_;                     ///< Device handle
	int formatterWidth_ = 28;            ///< Description field width, in characters (must be initialized or RegisterFormatter can resize to crazy large values!)


}; // end CFOandDTC_Registers class
}  // namespace DTCLib

#endif  // CFO_AND_DTC_REGISTERS_H