#ifndef DTC_REGISTERS_H
#define DTC_REGISTERS_H

//#include <bitset> // std::bitset
//#include <cstdint> // uint8_t, uint16_t
#include <functional>  // std::bind, std::function
#include <vector>      // std::vector

#include "DTC_Types.h"
#include "mu2edev.h"

namespace DTCLib {
enum DTC_Register : uint16_t
{
	DTC_Register_DesignVersion = 0x9000,
	DTC_Register_DesignDate = 0x9004,
	DTC_Register_DesignStatus = 0x9008,
	DTC_Register_VivadoVersion = 0x900C,
	DTC_Register_FPGA_Temperature = 0x9010,
	DTC_Register_FPGA_VCCINT = 0x9014,
	DTC_Register_FPGA_VCCAUX = 0x9018,
	DTC_Register_FPGA_VCCBRAM = 0x901C,
	DTC_Register_FPGA_MonitorAlarm = 0x9020,
	DTC_Register_DTCControl = 0x9100,
	DTC_Register_DMATransferLength = 0x9104,
	DTC_Register_SERDESLoopbackEnable = 0x9108,
	DTC_Register_ClockOscillatorStatus = 0x910C,
	DTC_Register_ROCEmulationEnable = 0x9110,
	DTC_Register_LinkEnable = 0x9114,
	DTC_Register_SERDES_Reset = 0x9118,
	DTC_Register_SERDES_RXDisparityError = 0x911C,
	DTC_Register_SERDES_RXCharacterNotInTableError = 0x9120,
	DTC_Register_SERDES_UnlockError = 0x9124,
	DTC_Register_SERDES_PLLLocked = 0x9128,
	DTC_Register_SERDES_PLLPowerDown = 0x912C,
	DTC_Register_SERDES_RXStatus = 0x9134,
	DTC_Register_SERDES_ResetDone = 0x9138,
	DTC_Register_SERDES_RXCDRLockStatus = 0x9140,
	DTC_Register_DMATimeoutPreset = 0x9144,
	DTC_Register_ROCReplyTimeout = 0x9148,
	DTC_Register_ROCReplyTimeoutError = 0x914C,
	DTC_Register_LinkPacketLength = 0x9150,
	DTC_Register_EVBPartitionID = 0x9154,
	DTC_Register_EVBDestCount = 0x9158,
	DTC_Register_SERDESTimingCardOscillatorFrequency = 0x915C,
	DTC_Register_SERDESMainBoardOscillatorFrequency = 0x9160,
	DTC_Register_SERDESOscillatorIICBusControl = 0x9164,
	DTC_Register_SERDESOscillatorIICBusLow = 0x9168,
	DTC_Register_SERDESOscillatorIICBusHigh = 0x916C,
	DTC_Register_DDROscillatorReferenceFrequency = 0x9170,
	DTC_Register_DDROscillatorIICBusControl = 0x9174,
	DTC_Register_DDROscillatorIICBusLow = 0x9178,
	DTC_Register_DDROscillatorIICBusHigh = 0x917C,
	DTC_Register_TimestampPreset0 = 0x9180,
	DTC_Register_TimestampPreset1 = 0x9184,
	DTC_Register_DataPendingTimer = 0x9188,
	DTC_Register_NUMROCs = 0x918C,
	DTC_Register_FIFOFullErrorFlag0 = 0x9190,
	DTC_Register_FIFOFullErrorFlag1 = 0x9194,
	DTC_Register_FIFOFullErrorFlag2 = 0x9198,
	DTC_Register_ReceivePacketError = 0x919C,
	DTC_Register_CFOEmulationTimestampLow = 0x91A0,
	DTC_Register_CFOEmulationTimestampHigh = 0x91A4,
	DTC_Register_CFOEmulationRequestInterval = 0x91A8,
	DTC_Register_CFOEmulationNumRequests = 0x91AC,
	DTC_Register_CFOEmulationNumPacketsLinks10 = 0x91B0,
	DTC_Register_CFOEmulationNumPacketsLinks32 = 0x91B4,
	DTC_Register_CFOEmulationNumPacketsLinks54 = 0x91B8,
	DTC_Register_CFOEmulationNumNullHeartbeats = 0x91BC,
	DTC_Register_CFOEmulationEventMode1 = 0x91C0,
	DTC_Register_CFOEmulationEventMode2 = 0x91C4,
	DTC_Register_CFOEmulationDebugPacketType = 0x91C8,
	DTC_Register_RXPacketCountErrorFlags = 0x91CC,
	DTC_Register_DetEmulationDMACount = 0x91D0,
	DTC_Register_DetEmulationDelayCount = 0x91D4,
	DTC_Register_DetEmulationControl0 = 0x91D8,
	DTC_Register_DetEmulationControl1 = 0x91DC,
	DTC_Register_DetEmulationDataStartAddress = 0x91E0,
	DTC_Register_DetEmulationDataEndAddress = 0x91E4,
	DTC_Register_ROCDRPDataSyncError = 0x91E8,
	DTC_Register_EthernetFramePayloadSize = 0x91EC,
	DTC_Register_CFOEmulationEventStartMarkerInterval = 0x91F0,
	DTC_Register_CFOEmulation40MHzClockMarkerInterval = 0x91F4,
	DTC_Register_CFOMarkerEnables = 0x91F8,
	DTC_Register_ReceiveByteCountDataLink0 = 0x9200,
	DTC_Register_ReceiveByteCountDataLink1 = 0x9204,
	DTC_Register_ReceiveByteCountDataLink2 = 0x9208,
	DTC_Register_ReceiveByteCountDataLink3 = 0x920C,
	DTC_Register_ReceiveByteCountDataLink4 = 0x9210,
	DTC_Register_ReceiveByteCountDataLink5 = 0x9214,
	DTC_Register_ReceiveByteCountDataCFO = 0x9218,
	DTC_Register_ReceiveByteCountDataEVB = 0x921C,
	DTC_Register_ReceivePacketCountDataLink0 = 0x9220,
	DTC_Register_ReceivePacketCountDataLink1 = 0x9224,
	DTC_Register_ReceivePacketCountDataLink2 = 0x9228,
	DTC_Register_ReceivePacketCountDataLink3 = 0x922C,
	DTC_Register_ReceivePacketCountDataLink4 = 0x9230,
	DTC_Register_ReceivePacketCountDataLink5 = 0x9234,
	DTC_Register_ReceivePacketCountDataCFO = 0x9238,
	DTC_Register_ReceivePacketCountDataEVB = 0x923C,
	DTC_Register_TransmitByteCountDataLink0 = 0x9240,
	DTC_Register_TransmitByteCountDataLink1 = 0x9244,
	DTC_Register_TransmitByteCountDataLink2 = 0x9248,
	DTC_Register_TransmitByteCountDataLink3 = 0x924C,
	DTC_Register_TransmitByteCountDataLink4 = 0x9250,
	DTC_Register_TransmitByteCountDataLink5 = 0x9254,
	DTC_Register_TransmitByteCountDataCFO = 0x9258,
	DTC_Register_TransmitByteCountDataEVB = 0x925C,
	DTC_Register_TransmitPacketCountDataLink0 = 0x9260,
	DTC_Register_TransmitPacketCountDataLink1 = 0x9264,
	DTC_Register_TransmitPacketCountDataLink2 = 0x9268,
	DTC_Register_TransmitPacketCountDataLink3 = 0x926C,
	DTC_Register_TransmitPacketCountDataLink4 = 0x9270,
	DTC_Register_TransmitPacketCountDataLink5 = 0x9274,
	DTC_Register_TransmitPacketCountDataCFO = 0x9278,
	DTC_Register_TransmitPacketCountDataEVB = 0x927C,
	DTC_Register_FireflyTXIICBusControl = 0x9284,
	DTC_Register_FireflyTXIICBusConfigLow = 0x9288,
	DTC_Register_FireflyTXIICBusConfigHigh = 0x928C,
	DTC_Register_FireflyRXIICBusControl = 0x9294,
	DTC_Register_FireflyRXIICBusConfigLow = 0x9298,
	DTC_Register_FireflyRXIICBusConfigHigh = 0x929C,
	DTC_Register_FireflyTXRXIICBusControl = 0x92A4,
	DTC_Register_FireflyTXRXIICBusConfigLow = 0x92A8,
	DTC_Register_FireflyTXRXIICBusConfigHigh = 0x92AC,
	DTC_Register_DDRLinkBufferFullFlags1 = 0x92B0,
	DTC_Register_DDRLinkBufferFullFlags2 = 0x92B4,
	DTC_Register_DDRLinkBufferFullErrorFlags1 = 0x92B8,
	DTC_Register_DDRLinkBufferFullErrorFlags2 = 0x92BC,
	DTC_Register_DDRLinkBufferEmptyFlags1 = 0x92C0,
	DTC_Register_DDRLinkBufferEmptyFlags2 = 0x92C4,
	DTC_Register_DDRLinkBufferHalfFullFlags1 = 0x92C8,
	DTC_Register_DDRLinkBufferHalfFullFlags2 = 0x92CC,
	DTC_Register_EventBuilderBufferFullFlags1 = 0x92D0,
	DTC_Register_EventBuilderBufferFullFlags2 = 0x92D4,
	DTC_Register_EventBuilderBufferFullErrorFlags1 = 0x92D8,
	DTC_Register_EventBuilderBufferFullErrorFlags2 = 0x92DC,
	DTC_Register_EventBuilderBufferEmptyFlags1 = 0x92E0,
	DTC_Register_EventBuilderBufferEmptyFlags2 = 0x92E4,
	DTC_Register_EventBuilderBufferHalfFullFlags1 = 0x92E8,
	DTC_Register_EventBuilderBufferHalfFullFlags2 = 0x92EC,
	DTC_Register_SERDESTXRXInvertEnable = 0x9300,
	DTC_Register_JitterAttenuatorCSR = 0x9308,
	DTC_Register_EVBSERDESPRBSControlStatus = 0x9330,
	DTC_Register_EventPadBytes = 0x9334,
	DTC_Register_MissedCFOPacketCountRing0 = 0x9340,
	DTC_Register_MissedCFOPacketCountRing1 = 0x9344,
	DTC_Register_MissedCFOPacketCountRing2 = 0x9348,
	DTC_Register_MissedCFOPacketCountRing3 = 0x934C,
	DTC_Register_MissedCFOPacketCountRing4 = 0x9350,
	DTC_Register_MissedCFOPacketCountRing5 = 0x9354,
	DTC_Register_LocalFragmentDropCount = 0x9360,
	DTC_Register_EventBuilderErrorFlags = 0x9370,
	DTC_Register_InputBufferErrorFlags = 0x9374,
	DTC_Register_OutputBufferErrorFlags = 0x9378,
	DTC_Register_Link0ErrorFlags = 0x937C,
	DTC_Register_Link1ErrorFlags = 0x9380,
	DTC_Register_Link2ErrorFlags = 0x9384,
	DTC_Register_Link3ErrorFlags = 0x9388,
	DTC_Register_Link4ErrorFlags = 0x938C,
	DTC_Register_Link5ErrorFlags = 0x9390,
	DTC_Register_Link6ErrorFlags = 0x9394,
	DTC_Register_CFOLinkErrorFlags = 0x9398,
	DTC_Register_LinkMuxErrorFlags = 0x939C,
	DTC_Register_FireFlyControlStatus = 0x93A0,
	DTC_Register_SFPControlStatus = 0x93A4,
	DTC_Register_FPGAProgramData = 0x9400,
	DTC_Register_FPGAPROMProgramStatus = 0x9404,
	DTC_Register_FPGACoreAccess = 0x9408,
	DTC_Register_EventModeLookupTableStart = 0x9500,
	DTC_Register_EventModeLookupTableEnd = 0x98FC,
	DTC_Register_Invalid,
};

/// <summary>
/// The DTC_Registers class represents the DTC Register space, and all the methods necessary to read and write those
/// registers. Each register has, at the very least, a read method, a write method, and a DTC_RegisterFormatter method
/// which formats the register value in a human-readable way.
/// </summary>
class DTC_Registers
{
public:
	/// <summary>
	/// Construct an instance of the DTC register map
	/// </summary>
	/// <param name="mode">Default: DTC_SimMode_Disabled; The simulation mode of the DTC</param>
	/// <param name="dtc">DTC card index to use</param>
	/// <param name="linkMask">Default 0x1; The initially-enabled links. Each digit corresponds to a link, so all links =
	/// 0x111111</param> <param name="skipInit">Default: false; Whether to skip initializing the DTC using the SimMode.
	/// Used to read state.</param> <param name="expectedDesignVersion">Expected DTC Firmware Design Version. If set, will
	/// throw an exception if the DTC firmware does not match (Default: "")</param>
	explicit DTC_Registers(DTC_SimMode mode, int dtc, std::string simFileName, unsigned linkMask = 0x1, std::string expectedDesignVersion = "",
						   bool skipInit = false);
	/// <summary>
	/// DTC_Registers destructor
	/// </summary>
	virtual ~DTC_Registers();

	/// <summary>
	/// Get a pointer to the device handle
	/// </summary>
	/// <returns>mu2edev* pointer</returns>
	mu2edev* GetDevice() { return &device_; }

	//
	// DTC Sim Mode Virtual Register
	//
	/// <summary>
	/// Get the current DTC_SimMode of this DTC_Registers object
	/// </summary>
	/// <returns></returns>
	DTC_SimMode ReadSimMode() const { return simMode_; }

	/// <summary>
	/// Initialize the DTC in the given SimMode.
	/// </summary>
	/// <param name="expectedDesignVersion">Expected DTC Firmware Design Version. If set, will throw an exception if the
	/// DTC firmware does not match</param> <param name="mode">Mode to set</param> <param name="dtc">DTC card index to
	/// use</param> <param name="linkMask">Default 0x1; The initially-enabled links. Each digit corresponds to a link, so
	/// all links = 0x111111</param> <param name="skipInit">Whether to skip initializing the DTC using the SimMode. Used
	/// to read state.</param> <returns></returns>
	DTC_SimMode SetSimMode(std::string expectedDesignVersion, DTC_SimMode mode, int dtc, std::string simMemoryFile, unsigned linkMask,
						   bool skipInit = false);

	//
	// DTC Register Dumps
	//
	/// <summary>
	/// Perform a register dump
	/// </summary>
	/// <param name="width">Printable width of description fields</param>
	/// <returns>String containing all registers, with their human-readable representations</returns>
	std::string FormattedRegDump(int width);
	/// <summary>
	/// Dump the link byte/packet counters
	/// </summary>
	/// <param name="width">Printable width of description fields</param>
	/// <returns>String containing the link counter registers, with their human-readable representations</returns>
	std::string LinkCountersRegDump(int width);

	/// <summary>
	/// Initializes a DTC_RegisterFormatter for the given DTC_Register
	/// </summary>
	/// <param name="address">Address of register to format</param>
	/// <returns>DTC_RegisterFormatter with address and raw value set</returns>
	DTC_RegisterFormatter CreateFormatter(const DTC_Register& address)
	{
		DTC_RegisterFormatter form;
		form.descWidth = formatterWidth_;
		form.address = address;
		form.value = ReadRegister_(address);
		return form;
	}

	//
	// Register IO Functions
	//

	// Desgin Version/Date Registers
	/// <summary>
	/// Read the design version
	/// </summary>
	/// <returns>Design version, in VersionNumber_Date format</returns>
	std::string ReadDesignVersion();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDesignVersion();
	/// <summary>
	/// Read the modification date of the DTC firmware
	/// </summary>
	/// <returns>Design date in 20YY-MM-DD-HH format</returns>
	std::string ReadDesignDate();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDesignDate();
	/// <summary>
	/// Read the design version number
	/// </summary>
	/// <returns>The design version number, in vMM.mm format</returns>
	std::string ReadDesignVersionNumber();

	// Design Status Register
	/// <summary>
	/// Read the DDR interface reset bit
	/// </summary>
	/// <returns>The current value of the DDR interface reset bit</returns>
	bool ReadDDRInterfaceReset();
	/// <summary>
	/// Set the Reset bit in the Design Status register
	/// </summary>
	/// <param name="reset"></param>
	void SetDDRInterfaceReset(bool reset);
	/// <summary>
	/// Resets the DDR interface in the firmware
	/// </summary>
	void ResetDDRInterface();

	/// <summary>
	/// Read the DDR Auto Calibration Done bit of the Design status register
	/// </summary>
	/// <returns>Whether the DDR Auto Calibration is done</returns>
	bool ReadDDRAutoCalibrationDone();

	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDesignStatus();

	// Vivado Version Register
	/// <summary>
	/// Read the Vivado Version Number
	/// </summary>
	/// <returns>The Vivado Version number</returns>
	std::string ReadVivadoVersionNumber();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatVivadoVersion();

	// FPGA Temperature Register
	/// <summary>
	/// Read the FPGA On-die Temperature sensor
	/// </summary>
	/// <returns>Temperature of the FGPA (deg. C)</returns>
	double ReadFPGATemperature();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGATemperature();

	// FPGA VCCINT Voltage Register
	/// <summary>
	/// Read the FPGA VCC INT Voltage (Nominal is 1.0 V)
	/// </summary>
	/// <returns>FPGA VCC INT Voltage (V)</returns>
	double ReadFPGAVCCINTVoltage();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGAVCCINT();

	// FPGA VCCAUX Voltage Register
	/// <summary>
	/// Read the FPGA VCC AUX Voltage (Nominal is 1.8 V)
	/// </summary>
	/// <returns>FPGA VCC AUX Voltage (V)</returns>
	double ReadFPGAVCCAUXVoltage();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGAVCCAUX();

	// FPGA VCCBRAM Voltage Register
	/// <summary>
	/// Read the FPGA VCC BRAM Voltage (Nominal 1.0 V)
	/// </summary>
	/// <returns>FPGA VCC BRAM Voltage (V)</returns>
	double ReadFPGAVCCBRAMVoltage();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGAVCCBRAM();

	// FPGA Monitor Alarm Register
	/// <summary>
	/// Read the value of the FPGA Die Temperature Alarm bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadFPGADieTemperatureAlarm();
	/// <summary>
	/// Reset the FPGA Die Temperature Alarm bit
	/// </summary>
	void ResetFPGADieTemperatureAlarm();
	/// <summary>
	/// Read the FPGA Alarms bit (OR of VCC and User Temperature alarm bits)
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadFPGAAlarms();
	/// <summary>
	/// Reset the FPGA Alarms bit
	/// </summary>
	void ResetFPGAAlarms();
	/// <summary>
	/// Read the VCC BRAM Alarm bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadVCCBRAMAlarm();
	/// <summary>
	/// Reset the VCC BRAM Alarm bit
	/// </summary>
	void ResetVCCBRAMAlarm();
	/// <summary>
	/// Read the VCC AUX Alarm bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadVCCAUXAlarm();
	/// <summary>
	/// Reset the VCC AUX Alarm bit
	/// </summary>
	void ResetVCCAUXAlarm();
	/// <summary>
	/// Read the VCC INT Alarm bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadVCCINTAlarm();
	/// <summary>
	/// Reset the VCC INT Alarm bit
	/// </summary>
	void ResetVCCINTAlarm();
	/// <summary>
	/// Read the FPGA User Temperature Alarm bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadFPGAUserTemperatureAlarm();
	/// <summary>
	/// Reset the FPGA User Temperature Alarm bit
	/// </summary>
	void ResetFPGAUserTemperatureAlarm();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGAAlarms();

	// DTC Control Register
	/// <summary>
	/// Perform a DTC Reset
	/// </summary>
	void ResetDTC();
	/// <summary>
	/// Read the Reset DTC Bit
	/// </summary>
	/// <returns>True if the DTC is currently resetting, false otherwise</returns>
	bool ReadResetDTC();
	/// <summary>
	/// Enable the DTC CFO Emulator
	/// Parameters for the CFO Emulator, such as count and starting timestamp, must be set before enabling.
	/// </summary>
	void EnableCFOEmulation();
	/// <summary>
	/// Disable the DTC CFO Emulator
	/// </summary>
	void DisableCFOEmulation();
	/// <summary>
	/// Reads the current state of the DTC CFO Emulator
	/// </summary>
	/// <returns>True if the emulator is enabled, false otherwise</returns>
	bool ReadCFOEmulation();
	/// <summary>
	/// Enable CFO Loopback. CFO packets will be returned to the CFO, for delay calculation.
	/// </summary>
	void EnableCFOLoopback();
	/// <summary>
	/// Disable CFO Loopback. CFO packets will be transmitted instead to the next DTC (Normal operation)
	/// </summary>
	void DisableCFOLoopback();
	/// <summary>
	/// Read the value of the CFO Link Loopback bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadCFOLoopback();
	/// <summary>
	/// Resets the DDR Write pointer to 0
	/// </summary>
	void ResetDDRWriteAddress();
	/// <summary>
	/// Determine whether a DDR Write Pointer reset is in progress
	/// </summary>
	/// <returns>True if the DDR Write pointer is resetting, false otherwise</returns>
	bool ReadResetDDRWriteAddress();
	/// <summary>
	/// Reset the DDR Read pointer to 0
	/// </summary>
	void ResetDDRReadAddress();
	/// <summary>
	/// Determine whether the DDR Read pointer is currently being reset.
	/// </summary>
	/// <returns>True if the read pointer is currently being reset, false otherwise</returns>
	bool ReadResetDDRReadAddress();
	/// <summary>
	/// Reset the DDR memory interface
	/// </summary>
	void ResetDDR();
	/// <summary>
	/// Determine whether the DDR memory interface is currently being reset
	/// </summary>
	/// <returns>True if the DDR memory interface is currently being reset, false otherwise</returns>
	bool ReadResetDDR();
	/// <summary>
	/// Enable sending Data Request packets from the DTC CFO Emulator with every
	/// Readout Request
	/// </summary>
	void EnableCFOEmulatorDRP();
	/// <summary>
	/// Disable sending Data Request packets from the DTC CFO Emulator with every
	/// Readout Request
	/// </summary>
	void DisableCFOEmulatorDRP();
	/// <summary>
	/// Read whether the DTC CFO Emulator is sending Data Request packets with every readout request
	/// </summary>
	/// <returns>True if the DTC CFO Emulator is sending Data Request packets with every readout request, false
	/// otherwise</returns>
	bool ReadCFOEmulatorDRP();
	/// <summary>
	/// Enable automatically generating Data Request packets from the DTC CFO Emulator
	/// </summary>
	void EnableAutogenDRP();
	/// <summary>
	/// Disable automatically generating Data Request packets from the DTC CFO Emulator
	/// </summary>
	void DisableAutogenDRP();
	/// <summary>
	/// Read whether Data Request packets are generated by the DTC CFO Emulator
	/// </summary>
	/// <returns>True if Data Request packets are generated by the DTC CFO Emulator, false otherwise</returns>
	bool ReadAutogenDRP();
	/// <summary>
	/// Enable receiving Data Request Packets from the DTCLib on DMA Channel 0
	/// Possibly obsolete, ask Rick before using
	/// </summary>
	void EnableSoftwareDRP();
	/// <summary>
	/// Disable receiving Data Request Packets from the DTCLib on DMA Channel 0
	/// Possibly obsolete, ask Rick before using
	/// </summary>
	void DisableSoftwareDRP();
	/// <summary>
	/// Read whether receiving Data Request Packets from the DTCLib on DMA Channel 0 is enabled
	/// </summary>
	/// <returns>True if receiving Data Request Packets from the DTCLib on DMA Channel 0 is enabled, false
	/// otherwise</returns>
	bool ReadSoftwareDRP();

	/// <summary>
	/// Enable the LED6 Control register bit
	/// </summary>
	void EnableLED6();
	/// <summary>
	/// Disable the LED6 Control register bit
	/// </summary>
	void DisableLED6();
	/// <summary>
	/// Read the state of the LED6 Control register bit
	/// </summary>
	/// <returns>Whether the LED6 bit is set</returns>
	bool ReadLED6State();
	/// <summary>
	/// Enable CFO Emulation mode. If CFO Emulation mode was not enabled, wait for links to become ready
	/// </summary>
	/// <param name="interval">Sleep interval while waiting for links</param>
	void SetCFOEmulationMode(size_t interval = 1000);
	/// <summary>
	/// Disable CFO Emulation mode. If CFO Emulation mode was senabled, wait for links to become ready
	/// </summary>
	/// <param name="interval">Sleep interval while waiting for links</param>
	void ClearCFOEmulationMode(size_t interval = 1000);
	/// <summary>
	/// Read the state of the CFO Emulation Mode bit
	/// </summary>
	/// <returns>Whether CFO Emulation Mode is enabled</returns>
	bool ReadCFOEmulationMode();
	/// <summary>
	/// Set the SERDES Global Reset bit to true, and wait for the reset to complete
	/// </summary>
	void ResetSERDES();
	/// <summary>
	/// Read the SERDES Global Reset bit
	/// </summary>
	/// <returns>Whether a SERDES global reset is in progress</returns>
	bool ReadResetSERDES();
	/// <summary>
	/// Enalbe receiving DCS packets.
	/// </summary>
	void EnableDCSReception();
	/// <summary>
	/// Disable receiving DCS packets. Any DCS packets received will be ignored
	/// </summary>
	void DisableDCSReception();
	/// <summary>
	/// Read the status of the DCS Enable bit
	/// </summary>
	/// <returns>Whether DCS packet reception is enabled</returns>
	bool ReadDCSReception();
	/// <summary>
	/// Set the DTC to External timing mode
	/// </summary>
	void SetExternalSystemClock();
	/// <summary>
	/// Set the DTC to Internal timing mode
	/// </summary>
	void SetInternalSystemClock();
	/// <summary>
	/// Read the status of the System timing bit
	/// </summary>
	/// <returns>True if the clock is in External mode, false for Internal mode</returns>
	bool ReadSystemClock();
	/// <summary>
	/// Enable Timing Synchronization for the ROC control packets
	/// </summary>
	void EnableTiming();
	/// <summary>
	/// Disable timing synchronization for ROC control packets
	/// </summary>
	void DisableTiming();
	/// <summary>
	/// Read the status of the timing sync enable bit
	/// </summary>
	/// <returns>The current value of the timing sync enable bit</returns>
	bool ReadTimingEnable();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDTCControl();

	// DMA Transfer Length Register
	/// <summary>
	/// Set the DMA buffer size which will automatically trigger a DMA
	/// </summary>
	/// <param name="length">Size, in bytes of buffer that will trigger a DMA</param>
	void SetTriggerDMATransferLength(uint16_t length);
	/// <summary>
	/// Read the DMA buffer size which will automatically trigger a DMA
	/// </summary>
	/// <returns>The DMA buffer size which will automatically trigger a DMA, in bytes</returns>
	uint16_t ReadTriggerDMATransferLength();
	/// <summary>
	/// Set the minimum DMA transfer size. Absolute minimum is 64 bytes.
	/// Buffers smaller than this size will be padded to the minimum.
	/// </summary>
	/// <param name="length">Size, in bytes, of the minimum DMA transfer buffer</param>
	void SetMinDMATransferLength(uint16_t length);
	/// <summary>
	/// Read the minimum DMA transfer size.
	/// </summary>
	/// <returns>The minimum DMA size, in bytes</returns>
	uint16_t ReadMinDMATransferLength();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDMATransferLength();

	// SERDES Loopback Enable Register
	/// <summary>
	/// Set the SERDES Loopback mode for the given link
	/// </summary>
	/// <param name="link">Link to set for</param>
	/// <param name="mode">DTC_SERDESLoopbackMode to set</param>
	void SetSERDESLoopbackMode(const DTC_Link_ID& link, const DTC_SERDESLoopbackMode& mode);
	/// <summary>
	/// Read the SERDES Loopback mode for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_SERDESLoopbackMode of the link</returns>
	DTC_SERDESLoopbackMode ReadSERDESLoopback(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESLoopbackEnable();

	// Clock Status Register
	/// <summary>
	/// Read the SERDES Oscillator IIC Error Bit
	/// </summary>
	/// <returns>True if the SERDES Oscillator IIC Error is set</returns>
	bool ReadSERDESOscillatorIICError();
	/// <summary>
	/// Read the DDR Oscillator IIC Error Bit
	/// </summary>
	/// <returns>True if the DDR Oscillator IIC Error is set</returns>
	bool ReadDDROscillatorIICError();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatClockOscillatorStatus();

	// ROC Emulation Enable Register
	/// <summary>
	/// Enable the ROC emulator on the given link
	/// Note that the ROC Emulator will use the NUMROCs register to determine how many ROCs to emulate
	/// </summary>
	/// <param name="link">Link to enable</param>
	void EnableROCEmulator(const DTC_Link_ID& link);
	/// <summary>
	/// Disable the ROC emulator on the given link
	/// </summary>
	/// <param name="link">Link to disable</param>
	void DisableROCEmulator(const DTC_Link_ID& link);
	/// <summary>
	/// Read the state of the ROC emulator on the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the ROC Emulator is enabled on the link</returns>
	bool ReadROCEmulator(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatROCEmulationEnable();

	// Link Enable Register
	/// <summary>
	/// Enable a SERDES Link
	/// </summary>
	/// <param name="link">Link to enable</param>
	/// <param name="mode">Link enable bits to set (Default: All)</param>
	void EnableLink(const DTC_Link_ID& link, const DTC_LinkEnableMode& mode = DTC_LinkEnableMode());
	/// <summary>
	/// Disable a SERDES Link
	/// The given mode bits will be UNSET
	/// </summary>
	/// <param name="link">Link to disable</param>
	/// <param name="mode">Link enable bits to unset (Default: All)</param>
	void DisableLink(const DTC_Link_ID& link, const DTC_LinkEnableMode& mode = DTC_LinkEnableMode());
	/// <summary>
	/// Read the Link Enable bits for a given SERDES link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_LinkEnableMode containing TX, RX, and CFO bits</returns>
	DTC_LinkEnableMode ReadLinkEnabled(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRingEnable();

	// SERDES Reset Register
	/// <summary>
	/// Reset the SERDES TX side
	/// Will poll the Reset SERDES TX Done flag until the SERDES reset is complete
	/// </summary>
	/// <param name="link">Link to reset</param>
	/// <param name="interval">Pollint interval, in microseconds</param>
	void ResetSERDESTX(const DTC_Link_ID& link, int interval = 100);
	/// <summary>
	/// Read if a SERDES TX reset is currently in progress
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if a SERDES TX reset is in progress</returns>
	bool ReadResetSERDESTX(const DTC_Link_ID& link);
	/// <summary>
	/// Reset the SERDES RX side
	/// Will poll the Reset SERDES RX Done flag until the SERDES reset is complete
	/// </summary>
	/// <param name="link">Link to reset</param>
	/// <param name="interval">Pollint interval, in microseconds</param>
	void ResetSERDESRX(const DTC_Link_ID& link, int interval = 100);
	/// <summary>
	/// Read if a SERDES RX reset is currently in progress
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if a SERDES reset is in progress</returns>
	bool ReadResetSERDESRX(const DTC_Link_ID& link);
	/// <summary>
	/// Reset the SERDES
	/// Will poll the Reset SERDES Done flag until the SERDES reset is complete
	/// </summary>
	/// <param name="link">Link to reset</param>
	/// <param name="interval">Pollint interval, in microseconds</param>
	void ResetSERDES(const DTC_Link_ID& link, int interval = 100);
	/// <summary>
	/// Read if a SERDES reset is currently in progress
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if a SERDES reset is in progress</returns>
	bool ReadResetSERDES(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESReset();

	// SERDES RX Disparity Error Register
	/// <summary>
	/// Read the SERDES RX Dispatity Error bits
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_SERDESRXDisparityError object with error bits</returns>
	DTC_SERDESRXDisparityError ReadSERDESRXDisparityError(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESRXDisparityError();

	// SERDES Character Not In Table Error Register
	/// <summary>
	/// Read the SERDES Character Not In Table Error bits
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_CharacterNotInTableError object with error bits</returns>
	DTC_CharacterNotInTableError ReadSERDESRXCharacterNotInTableError(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESRXCharacterNotInTableError();

	// SERDES Unlock Error Register
	/// <summary>
	/// Read whether the SERDES Unlock Error bit is set
	/// </summary>
	/// <param name="link">Link to check</param>
	/// <returns>True if the SERDES Unlock Error bit is set on the given link</returns>
	bool ReadSERDESUnlockError(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESUnlockError();

	// SERDES PLL Locked Register
	/// <summary>
	/// Read if the SERDES PLL is locked for the given SERDES link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the PLL is locked, false otherwise</returns>
	bool ReadSERDESPLLLocked(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESPLLLocked();

	// SERDES PLL Power Down
	/// <summary>
	/// Disable the SERDES PLL Power Down bit for the given link, enabling that link
	/// </summary>
	/// <param name="link">Link to set</param>
	void EnableSERDESPLL(const DTC_Link_ID& link);
	/// <summary>
	/// Enable the SERDES PLL Power Down bit for the given link, disabling that link
	/// </summary>
	/// <param name="link">Link to set</param>
	void DisableSERDESPLL(const DTC_Link_ID& link);
	/// <summary>
	/// Read the SERDES PLL Power Down bit for the given link
	/// </summary>
	/// <param name="link">link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESPLLPowerDown(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESPLLPowerDown();

	// SERDES RX Status Register
	/// <summary>
	/// Read the SERDES RX Status for the given SERDES Link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_RXStatus object</returns>
	DTC_RXStatus ReadSERDESRXStatus(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESRXStatus();

	// SERDES Reset Done Register
	/// <summary>
	/// Read the SERDES Reset RX FSM Done bit
	/// </summary>
	/// <param name="link">link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadResetRXFSMSERDESDone(const DTC_Link_ID& link);
	/// <summary>
	/// Read the SERDES Reset RX Done bit
	/// </summary>
	/// <param name="link">link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadResetRXSERDESDone(const DTC_Link_ID& link);
	/// <summary>
	/// Read the SERDES Reset TX FSM Done bit
	/// </summary>
	/// <param name="link">link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadResetTXFSMSERDESDone(const DTC_Link_ID& link);
	/// <summary>
	/// Read the SERDES Reset TX Done bit
	/// </summary>
	/// <param name="link">link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadResetTXSERDESDone(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESResetDone();

	// SERDES CDR Lock Register
	/// <summary>
	/// Read the SERDES CDR Lock bit for the given SERDES Link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the SERDES CDR Lock bit is set</returns>
	bool ReadSERDESRXCDRLock(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRXCDRLockStatus();

	// DMA Timeout Preset Regsiter
	/// <summary>
	/// Set the maximum time a DMA buffer may be active before it is sent, in 4ns ticks.
	/// The default value is 0x800
	/// </summary>
	/// <param name="preset">Maximum active time for DMA buffers</param>
	void SetDMATimeoutPreset(uint32_t preset);
	/// <summary>
	/// Read the maximum time a DMA buffer may be active before it is sent, in 4ns ticks.
	/// The default value is 0x800
	/// </summary>
	/// <returns>Maximum active time for DMA buffers</returns>
	uint32_t ReadDMATimeoutPreset();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDMATimeoutPreset();

	// ROC Timeout (Header Packet to All Packets Received) Preset Register
	/// <summary>
	/// Set the timeout between the reception of a Data Header packet from a ROC and receiving all of the associated Data
	/// Packets. If a timeout occurrs, the ROCTimeoutError flag will be set. Timeout is in SERDES clock ticks
	/// </summary>
	/// <param name="preset">Timeout value. Default: 0x200000</param>
	void SetROCTimeoutPreset(uint32_t preset);
	/// <summary>
	/// Read the timeout between the reception of a Data Header packet from a ROC and receiving all of the associated Data
	/// Packets. If a timeout occurrs, the ROCTimeoutError flag will be set. Timeout is in SERDES clock ticks
	/// </summary>
	/// <returns>Timeout value</returns>
	uint32_t ReadROCTimeoutPreset();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatROCReplyTimeout();

	// ROC Timeout Error Register
	/// <summary>
	/// Clear the ROC Data Packet timeout error flag for the given SERDES link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearROCTimeoutError(const DTC_Link_ID& link);
	/// <summary>
	/// Read the ROC Data Packet Timeout Error Flag for the given SERDES link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the error flag is set, false otherwise</returns>
	bool ReadROCTimeoutError(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatROCReplyTimeoutError();

	// Link Packet Length Register
	/// <summary>
	/// Set the size of DTC SERDES packets. Default is 16 bytes
	/// This value should most likely never be changed.
	/// </summary>
	/// <param name="packetSize">New packet size, in bytes</param>
	void SetPacketSize(uint16_t packetSize);
	/// <summary>
	/// Read the size of DTC SERDES packets. Default is 16 bytes
	/// </summary>
	/// <returns>Packet size, in bytes</returns>
	uint16_t ReadPacketSize();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRingPacketLength();

	// EVB Network Partition ID / EVB Network Local MAC Index Register
	/// <summary>
	/// Set the EVB Mode byte
	/// </summary>
	/// <param name="mode">New Mode byte</param>
	void SetEVBMode(uint8_t mode);
	/// <summary>
	/// Read the EVB Mode byte
	/// </summary>
	/// <returns>EVB Mode byte</returns>
	uint8_t ReadEVBMode();
	/// <summary>
	/// Set the local partition ID
	/// </summary>
	/// <param name="id">Local partition ID</param>
	void SetEVBLocalParitionID(uint8_t id);
	/// <summary>
	/// Read the local partition ID
	/// </summary>
	/// <returns>Partition ID</returns>
	uint8_t ReadEVBLocalParitionID();
	/// <summary>
	/// Set the MAC address for the EVB network (lowest byte)
	/// </summary>
	/// <param name="macByte">MAC Address</param>
	void SetEVBLocalMACAddress(uint8_t macByte);
	/// <summary>
	/// Read the MAC address for the EVB network (lowest byte)
	/// </summary>
	/// <returns>MAC Address</returns>
	uint8_t ReadEVBLocalMACAddress();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatEVBLocalParitionIDMACIndex();

	// EVB Number of Destination Nodes Register
	/// <summary>
	/// Set the start node in the EVB cluster
	/// </summary>
	/// <param name="node">Node ID (MAC Address)</param>
	void SetEVBStartNode(uint8_t node);
	/// <summary>
	/// Read the start node in the EVB cluster
	/// </summary>
	/// <returns>Node ID (MAC Address)</returns>
	uint8_t ReadEVBStartNode();
	/// <summary>
	/// Set the number of destination nodes in the EVB cluster
	/// </summary>
	/// <param name="number">Number of nodes</param>
	void SetEVBNumberOfDestinationNodes(uint8_t number);
	/// <summary>
	/// Read the number of destination nodes in the EVB cluster
	/// </summary>
	/// <returns>Number of nodes</returns>
	uint8_t ReadEVBNumberOfDestinationNodes();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatEVBNumberOfDestinationNodes();

	// SERDES Oscillator Registers
	/// <summary>
	/// Read the current SERDES Oscillator reference frequency, in Hz
	/// </summary>
	/// <param name="device">Device to set oscillator for</param>
	/// <returns>Current SERDES Oscillator reference frequency, in Hz</returns>
	uint32_t ReadSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress device);
	/// <summary>
	/// Set the SERDES Oscillator reference frequency
	/// </summary>
	/// <param name="device">Device to set oscillator for</param>
	/// <param name="freq">New reference frequency, in Hz</param>
	void SetSERDESOscillatorReferenceFrequency(DTC_IICSERDESBusAddress device, uint32_t freq);

	/// <summary>
	/// Read the Reset bit of the SERDES IIC Bus
	/// </summary>
	/// <returns>Reset bit value</returns>
	bool ReadSERDESOscillatorIICInterfaceReset();
	/// <summary>
	/// Reset the SERDES IIC Bus
	/// </summary>
	void ResetSERDESOscillatorIICInterface();

	/// <summary>
	/// Write a value to the SERDES IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <param name="data">Data to write</param>
	void WriteSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address, uint8_t data);
	/// <summary>
	/// Read a value from the SERDES IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <returns>Value of register</returns>
	uint8_t ReadSERDESIICInterface(DTC_IICSERDESBusAddress device, uint8_t address);

	/// <summary>
	/// Read the current SERDES Oscillator clock speed
	/// </summary>
	/// <returns>Current SERDES clock speed</returns>
	DTC_SerdesClockSpeed ReadSERDESOscillatorClock();
	/// <summary>
	/// Set the SERDES Oscillator clock speed for the given SERDES transfer rate
	/// </summary>
	/// <param name="speed">Clock speed to set</param>
	void SetSERDESOscillatorClock(DTC_SerdesClockSpeed speed);
	/// <summary>
	/// Set the Timing Oscillator clock to a given frequency
	/// </summary>
	/// <param name="freq">Frequency to set the Timing card Oscillator clock</param>
	void SetTimingOscillatorClock(uint32_t freq);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTimingSERDESOscillatorFrequency();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMainBoardSERDESOscillatorFrequency();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESOscillatorControl();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESOscillatorParameterLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESOscillatorParameterHigh();

	// DDR Oscillator Registers
	/// <summary>
	/// Read the current DDR Oscillator frequency, in Hz
	/// </summary>
	/// <returns>Current DDR Oscillator frequency, in Hz</returns>
	uint32_t ReadDDROscillatorReferenceFrequency();
	/// <summary>
	/// Set the DDR Oscillator frequency
	/// </summary>
	/// <param name="freq">New frequency, in Hz</param>
	void SetDDROscillatorReferenceFrequency(uint32_t freq);
	/// <summary>
	/// Read the Reset bit of the DDR IIC Bus
	/// </summary>
	/// <returns>Reset bit value</returns>
	bool ReadDDROscillatorIICInterfaceReset();
	/// <summary>
	/// Reset the DDR IIC Bus
	/// </summary>
	void ResetDDROscillatorIICInterface();

	/// <summary>
	/// Write a value to the DDR IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <param name="data">Data to write</param>
	void WriteDDRIICInterface(DTC_IICDDRBusAddress device, uint8_t address, uint8_t data);
	/// <summary>
	/// Read a value from the DDR IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <returns>Value of register</returns>
	uint8_t ReadDDRIICInterface(DTC_IICDDRBusAddress device, uint8_t address);

	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDROscillatorFrequency();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDROscillatorControl();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDROscillatorParameterLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDROscillatorParameterHigh();

	// Timestamp Preset Registers
	/// <summary>
	/// Set the Timestamp preset for Timing system emulation mode
	/// </summary>
	/// <param name="preset">Timestamp for Timing emulation</param>
	void SetTimestampPreset(const DTC_Timestamp& preset);
	/// <summary>
	/// Read the Timestamp preset for Timing system emulation mode
	/// </summary>
	/// <returns>Timestamp preset</returns>
	DTC_Timestamp ReadTimestampPreset();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTimestampPreset0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTimestampPreset1();

	// Data Pending Timer Register
	/// <summary>
	/// Set the timeout for waiting for a reply after sending a Data Request packet.
	/// Timeout is in SERDES clock ticks. Default value is 0x2000
	/// </summary>
	/// <param name="timer">New timeout</param>
	void SetDataPendingTimer(uint32_t timer);
	/// <summary>
	/// Read the timeout for waiting for a reply after sending a Data Request packet.
	/// Timeout is in SERDES clock ticks. Default value is 0x2000
	/// </summary>
	/// <returns>Current timeout</returns>
	uint32_t ReadDataPendingTimer();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDataPendingTimer();

	// NUMROCs Register
	/// <summary>
	/// Set the maximum ROC ID for the given link
	/// </summary>
	/// <param name="link">Link to set</param>
	/// <param name="lastRoc">ID of the last ROC in the link</param>
	void SetMaxROCNumber(const DTC_Link_ID& link, const uint8_t& lastRoc);
	/// <summary>
	/// Read the number of ROCs configured on the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>ID of last ROC on link</returns>
	uint8_t ReadLinkROCCount(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatNUMROCs();

	// FIFO Full Error Flags Registers
	/// <summary>
	/// Clear all FIFO Full Error Flags for the given link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearFIFOFullErrorFlags(const DTC_Link_ID& link);
	/// <summary>
	/// Read the FIFO Full Error/Status Flags for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>DTC_FIFOFullErrorFlags object</returns>
	DTC_FIFOFullErrorFlags ReadFIFOFullErrorFlags(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFIFOFullErrorFlag0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFIFOFullErrorFlag1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFIFOFullErrorFlag2();

	// Receive Packet Error Register
	/// <summary>
	/// Clear the Packet Error Flag for the given link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearPacketError(const DTC_Link_ID& link);
	/// <summary>
	/// Read the Packet Error Flag for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the Packet Error Flag is set</returns>
	bool ReadPacketError(const DTC_Link_ID& link);
	/// <summary>
	/// Clear the Packet CRC Error Flag for the given link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearPacketCRCError(const DTC_Link_ID& link);
	/// <summary>
	/// Read the Packet CRC Error Flag for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the Packet CRC Error Flag is set</returns>
	bool ReadPacketCRCError(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketError();

	// CFO Emulation Timestamp Registers
	/// <summary>
	/// Set the starting DTC_Timestamp for the CFO Emulator
	/// </summary>
	/// <param name="ts">Starting Timestamp for CFO Emulation</param>
	void SetCFOEmulationTimestamp(const DTC_Timestamp& ts);
	/// <summary>
	/// Read the starting DTC_Timestamp for the CFO Emulator
	/// </summary>
	/// <returns>DTC_Timestamp object</returns>
	DTC_Timestamp ReadCFOEmulationTimestamp();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationTimestampLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationTimestampHigh();

	// CFO Emulation Request Interval Regsister
	/// <summary>
	/// Set the clock interval between CFO Emulator Readout Requests.
	/// This value is dependent upon the SERDES clock speed (2.5 Gbps = 125 MHz, 3.125 Gbps = 156.25 MHz)
	/// </summary>
	/// <param name="interval">Clock cycles between Readout Requests</param>
	void SetCFOEmulationRequestInterval(uint32_t interval);
	/// <summary>
	/// Read the clock interval between CFO Emulator Readout Requests.
	/// This value is dependent upon the SERDES clock speed (2.5 Gbps = 125 MHz, 3.125 Gbps = 156.25 MHz)
	/// </summary>
	/// <returns>Clock cycles between Readout Requests</returns>
	uint32_t ReadCFOEmulationRequestInterval();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationRequestInterval();

	// CFO Emulation Number of Requests Register
	/// <summary>
	/// Set the number of Readout Requests the CFO Emulator is configured to send.
	/// A value of 0 means that the CFO Emulator will send requests continuously.
	/// </summary>
	/// <param name="numRequests">Number of Readout Requests the CFO Emulator will send</param>
	void SetCFOEmulationNumRequests(uint32_t numRequests);
	/// <summary>
	/// Reads the number of Readout Requests the CFO Emulator is configured to send.
	/// A value of 0 means that the CFO Emulator will send requests continuously.
	/// </summary>
	/// <returns>Number of Readout Requests the CFO Emulator will send</returns>
	uint32_t ReadCFOEmulationNumRequests();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationNumRequests();

	// CFO Emulation Number of Packets Registers
	/// <summary>
	/// Set the number of packets the CFO Emulator will request from the link
	/// </summary>
	/// <param name="link">Link to set</param>
	/// <param name="numPackets">Number of packets to request</param>
	void SetCFOEmulationNumPackets(const DTC_Link_ID& link, uint16_t numPackets);
	/// <summary>
	/// Read the requested number of packets the CFO Emulator will request from the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Number of packets requested from the link</returns>
	uint16_t ReadCFOEmulationNumPackets(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationNumPacketsLink01();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationNumPacketsLink23();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationNumPacketsLink45();

	// CFO Emulation Number of Null Heartbeats Register
	/// <summary>
	/// Set the number of null heartbeats the CFO Emulator will generate following the requested ones
	/// </summary>
	/// <param name="count">Number of null heartbeats to generate</param>
	void SetCFOEmulationNumNullHeartbeats(const uint32_t& count);
	/// <summary>
	/// Read the requested number of null heartbeats that will follow the configured heartbeats from the CFO Emulator
	/// </summary>
	/// <returns>Number of null heartbeats to follow "live" heartbeats</returns>
	uint32_t ReadCFOEmulationNumNullHeartbeats();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationNumNullHeartbeats();

	// CFO Emulation Event Mode Bytes Registers
	/// <summary>
	/// Set the given CFO Emulation Mode byte to the given value
	/// </summary>
	/// <param name="byteNum">Byte to set. Valid range is 0-5</param>
	/// <param name="data">Data for byte</param>
	void SetCFOEmulationModeByte(const uint8_t& byteNum, uint8_t data);
	/// <summary>
	/// Read the given CFO Emulation Mode byte
	/// </summary>
	/// <param name="byteNum">Byte to read. Valid range is 0-5</param>
	/// <returns>Current value of the mode byte</returns>
	uint8_t ReadCFOEmulationModeByte(const uint8_t& byteNum);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationModeBytes03();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationModeBytes45();

	// CFO Emulation Debug Packet Type Register
	/// <summary>
	/// Enable putting the Debug Mode in Readout Requests
	/// </summary>
	void EnableDebugPacketMode();
	/// <summary>
	/// Disable putting the Debug Mode in Readout Requests
	/// </summary>
	void DisableDebugPacketMode();
	/// <summary>
	/// Whether Debug mode packets are enabled
	/// </summary>
	/// <returns>True if Debug Mode is enabled</returns>
	bool ReadDebugPacketMode();
	/// <summary>
	/// Set the DebugType used by the CFO Emulator
	/// </summary>
	/// <param name="type">The DTC_DebugType the CFO Emulator will fill into Readout Requests</param>
	void SetCFOEmulationDebugType(DTC_DebugType type);
	/// <summary>
	/// Read the DebugType field filled into Readout Requests generated by the CFO Emulator
	/// </summary>
	/// <returns>The DTC_DebugType used by the CFO Emulator</returns>
	DTC_DebugType ReadCFOEmulationDebugType();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOEmulationDebugPacketType();

	// RX Packet Count Error Flags Register
	/// <summary>
	/// Read the RX Packet Count Error flag for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Whether the RX Packet Count Error flag is set on the link</returns>
	bool ReadRXPacketCountErrorFlags(const DTC_Link_ID& link);
	/// <summary>
	/// Clear the RX Packet Count Error flag for the given link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearRXPacketCountErrorFlags(const DTC_Link_ID& link);
	/// <summary>
	/// Clear all RX Packet Count Error Flags
	/// </summary>
	void ClearRXPacketCountErrorFlags();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRXPacketCountErrorFlags();

	// Detector Emulation DMA Count Register
	/// <summary>
	/// Set the number of DMAs that the Detector Emulator will generate when enabled
	/// </summary>
	/// <param name="count">The number of DMAs that the Detector Emulator will generate when enabled</param>
	void SetDetectorEmulationDMACount(uint32_t count);
	/// <summary>
	/// Read the number of DMAs that the Detector Emulator will generate
	/// </summary>
	/// <returns>The number of DMAs that the Detector Emulator will generate</returns>
	uint32_t ReadDetectorEmulationDMACount();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDetectorEmulationDMACount();

	// Detector Emulation DMA Delay Count Register
	/// <summary>
	/// Set the delay between DMAs in Detector Emulator mode
	/// </summary>
	/// <param name="count">Delay between DMAs in Detector Emulation mode, in 4ns ticks</param>
	void SetDetectorEmulationDMADelayCount(uint32_t count);
	/// <summary>
	/// Read the amount of the delay between DMAs in Detector Emulator mode
	/// </summary>
	/// <returns>The amount of the delay between DMAs in Detector Emulator mode, in 4ns ticks</returns>
	uint32_t ReadDetectorEmulationDMADelayCount();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDetectorEmulationDMADelayCount();

	// Detector Emulation Control Registers
	/// <summary>
	/// Enable Detector Emulator Mode. This sends all DMA writes to DMA channel 0 (DAQ) to DDR memory
	/// </summary>
	void EnableDetectorEmulatorMode();
	/// <summary>
	/// Disable sending DMA data to DDR memory
	/// </summary>
	void DisableDetectorEmulatorMode();
	/// <summary>
	/// Read whether writes to DMA Channel 0 will be loaded into DDR memory
	/// </summary>
	/// <returns>Whether the Detector Emulator Mode bit is set</returns>
	bool ReadDetectorEmulatorMode();
	/// <summary>
	/// Enable the Detector Emulator (Playback Mode)
	/// This assumes that data has been loaded into DDR memory using DMA Channel 0 before enabling.
	/// </summary>
	void EnableDetectorEmulator();
	/// <summary>
	/// Turn off the Detector Emulator (Playback Mode)
	/// </summary>
	void DisableDetectorEmulator();
	/// <summary>
	/// Read whether the Detector Emulator is enabled
	/// </summary>
	/// <returns>Whether the Detector Emulator is enabled</returns>
	bool ReadDetectorEmulatorEnable();
	/// <summary>
	/// Read whether a Detector Emulator Disable operation is in progress
	/// </summary>
	/// <returns>Whether the Detector Emulator Enable Clear bit is set</returns>
	bool ReadDetectorEmulatorEnableClear();
	/// <summary>
	/// Return the current value of the "Detector Emulator In Use" virtual register
	/// </summary>
	/// <returns>Whether the DTC Detector Emulator has been set up</returns>
	bool IsDetectorEmulatorInUse() const { return usingDetectorEmulator_; }
	/// <summary>
	/// Set the "Detector Emulator In Use" virtual register to true
	/// </summary>
	void SetDetectorEmulatorInUse() {
		TLOG(TLVL_WARNING) << "DTC_Registers::SetDetectorEmulatorInUse: Enabling Detector Emulator!";
		usingDetectorEmulator_ = true; }
	/// <summary>
	/// Clear the "Detector Emulator In Use" virtual register
	/// </summary>
	void ClearDetectorEmulatorInUse();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDetectorEmulationControl0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDetectorEmulationControl1();

	// DDR Event Data Local Start Address Register
	/// <summary>
	/// Set the DDR Data Start Address
	/// DDR Addresses are in bytes and must be 64-bit aligned
	/// </summary>
	/// <param name="address">Start address for the DDR data section</param>
	void SetDDRDataLocalStartAddress(uint32_t address);
	/// <summary>
	/// Read the DDR Data Start Address
	/// </summary>
	/// <returns>The current DDR data start address</returns>
	uint32_t ReadDDRDataLocalStartAddress();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRDataLocalStartAddress();

	// DDR Event Data Local End Address Register
	/// <summary>
	/// Set the end address for the DDR data section
	/// DDR Addresses are in bytes and must be 64-bit aligned
	/// </summary>
	/// <param name="address"></param>
	void SetDDRDataLocalEndAddress(uint32_t address);
	/// <summary>
	/// Read the current end address for the DDR Data section
	/// </summary>
	/// <returns>End address for the DDR data section</returns>
	uint32_t ReadDDRDataLocalEndAddress();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRDataLocalEndAddress();

	// ROC DRP Sync Error Register
	/// <summary>
	/// Read the ROC DRP Sync Error Flag for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>True if the ROC DRP Sync Error Flag is set on the given link, false otherwise</returns>
	bool ReadROCDRPSyncErrors(const DTC_Link_ID& link);
	/// <summary>
	/// Clear ROC DRP Sync Errors for the given link
	/// </summary>
	/// <param name="link">Link to clear</param>
	void ClearROCDRPSyncErrors(const DTC_Link_ID& link);
	/// <summary>
	/// Clears all ROC DRP Sync Errors
	/// </summary>
	void ClearROCDRPSyncErrors();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatROCDRPSyncError();

	// Ethernet Frame Payload Max Size
	/// <summary>
	/// Read the current maximum Ethernet payload size
	/// </summary>
	/// <returns>The current maximum Ethernet payload size</returns>
	uint32_t ReadEthernetPayloadSize();
	/// <summary>
	/// Set the maximum Ethernet payload size, in bytes. Maximum is 1492 bytes.
	/// </summary>
	/// <param name="size">Maximum Ethernet payload size, in bytes</param>
	void SetEthernetPayloadSize(uint32_t size);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatEthernetPayloadSize();

	// SERDES Counter Registers
	/// <summary>
	/// Clear the value of the Receive byte counter
	/// </summary>
	/// <param name="link">Link to clear counter for</param>
	void ClearReceiveByteCount(const DTC_Link_ID& link);
	/// <summary>
	/// Read the value of the Receive byte counter
	/// </summary>
	/// <param name="link">Link to read counter for</param>
	/// <returns>Current value of the Receive byte counter on the given link</returns>
	uint32_t ReadReceiveByteCount(const DTC_Link_ID& link);
	/// <summary>
	/// Clear the value of the Receive Packet counter
	/// </summary>
	/// <param name="link">Link to clear counter for</param>
	void ClearReceivePacketCount(const DTC_Link_ID& link);
	/// <summary>
	/// Read the value of the Receive Packet counter
	/// </summary>
	/// <param name="link">Link to read counter for</param>
	/// <returns>Current value of the Receive Packet counter on the given link</returns>
	uint32_t ReadReceivePacketCount(const DTC_Link_ID& link);
	/// <summary>
	/// Clear the value of the Transmit byte counter
	/// </summary>
	/// <param name="link">Link to clear counter for</param>
	void ClearTransmitByteCount(const DTC_Link_ID& link);
	/// <summary>
	/// Read the value of the Transmit byye counter
	/// </summary>
	/// <param name="link">Link to read counter for</param>
	/// <returns>Current value of the Transmit byte counter on the given link</returns>
	uint32_t ReadTransmitByteCount(const DTC_Link_ID& link);
	/// <summary>
	/// Clear the value of the Transmit Packet counter
	/// </summary>
	/// <param name="link">Link to clear counter for</param>
	void ClearTransmitPacketCount(const DTC_Link_ID& link);
	/// <summary>
	/// Read the value of the Transmit Packet counter
	/// </summary>
	/// <param name="link">Link to read counter for</param>
	/// <returns>Current value of the Transmit Packet counter on the given link</returns>
	uint32_t ReadTransmitPacketCount(const DTC_Link_ID& link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing2();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing3();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing4();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountRing5();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountCFO();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceiveByteCountEVB();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing2();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing3();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing4();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountRing5();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountCFO();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatReceivePacketCountEVB();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing2();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing3();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing4();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountRing5();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountCFO();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTramsitByteCountEVB();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing2();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing3();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing4();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountRing5();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountCFO();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatTransmitPacketCountEVB();

	// Firefly TX IIC Registers
	/// <summary>
	/// Read the Reset bit of the Firefly TX IIC Bus
	/// </summary>
	/// <returns>Reset bit value</returns>
	bool ReadFireflyTXIICInterfaceReset();
	/// <summary>
	/// Reset the Firefly TX IIC Bus
	/// </summary>
	void ResetFireflyTXIICInterface();

	/// <summary>
	/// Write a value to the Firefly TX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <param name="data">Data to write</param>
	void WriteFireflyTXIICInterface(uint8_t device, uint8_t address, uint8_t data);
	/// <summary>
	/// Read a value from the Firefly TX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <returns>Value of register</returns>
	uint8_t ReadFireflyTXIICInterface(uint8_t device, uint8_t address);

	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXIICControl();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXIICParameterLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXIICParameterHigh();

	// Firefly RX IIC Registers
	/// <summary>
	/// Read the Reset bit of the Firefly RX IIC Bus
	/// </summary>
	/// <returns>Reset bit value</returns>
	bool ReadFireflyRXIICInterfaceReset();
	/// <summary>
	/// Reset the Firefly RX IIC Bus
	/// </summary>
	void ResetFireflyRXIICInterface();

	/// <summary>
	/// Write a value to the Firefly RX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <param name="data">Data to write</param>
	void WriteFireflyRXIICInterface(uint8_t device, uint8_t address, uint8_t data);
	/// <summary>
	/// Read a value from the Firefly RX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <returns>Value of register</returns>
	uint8_t ReadFireflyRXIICInterface(uint8_t device, uint8_t address);

	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyRXIICControl();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyRXIICParameterLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyRXIICParameterHigh();

	// Firefly TXRX IIC Registers
	/// <summary>
	/// Read the Reset bit of the Firefly TXRX IIC Bus
	/// </summary>
	/// <returns>Reset bit value</returns>
	bool ReadFireflyTXRXIICInterfaceReset();
	/// <summary>
	/// Reset the Firefly TXRX IIC Bus
	/// </summary>
	void ResetFireflyTXRXIICInterface();

	/// <summary>
	/// Write a value to the Firefly TXRX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <param name="data">Data to write</param>
	void WriteFireflyTXRXIICInterface(uint8_t device, uint8_t address, uint8_t data);
	/// <summary>
	/// Read a value from the Firefly TXRX IIC Bus
	/// </summary>
	/// <param name="device">Device address</param>
	/// <param name="address">Register address</param>
	/// <returns>Value of register</returns>
	uint8_t ReadFireflyTXRXIICInterface(uint8_t device, uint8_t address);

	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXRXIICControl();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXRXIICParameterLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyTXRXIICParameterHigh();

	// DDR Memory Flags Registers
	/// <summary>
	/// Read the DDR Flags for the given buffer
	/// </summary>
	/// <param name="buffer_id">Buffer number to read (0-63)</param>
	/// <returns>Value of the DDR Flags for the given buffer</returns>
	DTC_DDRFlags ReadDDRFlags(uint8_t buffer_id);
	/// <summary>
	/// Read the DDR Link Buffer Full Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with full status of each of the buffers</returns>
	std::bitset<64> ReadDDRLinkBufferFullFlags();
	/// <summary>
	/// Read the DDR Link Buffer Full Error Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with full error status of each of the buffers</returns>
	std::bitset<64> ReadDDRLinkBufferFullErrorFlags();
	/// <summary>
	/// Read the DDR Link Buffer Empty Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with empty status of each of the buffers</returns>
	std::bitset<64> ReadDDRLinkBufferEmptyFlags();
	/// <summary>
	/// Read the DDR Link Buffer Half-Full Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with half-full status of each of the buffers</returns>
	std::bitset<64> ReadDDRLinkBufferHalfFullFlags();
	/// <summary>
	/// Read the DDR EVB Buffer Full Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with full status of each of the buffers</returns>
	std::bitset<64> ReadDDREventBuilderBufferFullFlags();
	/// <summary>
	/// Read the DDR EVB Buffer Full Error Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with full error status of each of the buffers</returns>
	std::bitset<64> ReadDDREventBuilderBufferFullErrorFlags();
	/// <summary>
	/// Read the DDR EVB Buffer Empty Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with empty status of each of the buffers</returns>
	std::bitset<64> ReadDDREventBuilderBufferEmptyFlags();
	/// <summary>
	/// Read the DDR EVB Buffer Half-Full Flags for each of the 64 DDR buffers
	/// </summary>
	/// <returns>64-bit bitset with half-full status of each of the buffers</returns>
	std::bitset<64> ReadDDREventBuilderBufferHalfFullFlags();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferFullFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferFullErrorFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferEmptyFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferHalfFullFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferFullFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferFullErrorFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferEmptyFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferHalfFullFlagsLow();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferFullFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferFullErrorFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferEmptyFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDRLinkBufferHalfFullFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferFullFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferFullErrorFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferEmptyFlagsHigh();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatDDREventBuilderBufferHalfFullFlagsHigh();

	// SERDES Serial Inversion Enable Register
	/// <summary>
	/// Read the Invert SERDES RX Input bit
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Whether the Invert SERDES RX Input bit is set</returns>
	bool ReadInvertSERDESRXInput(DTC_Link_ID link);
	/// <summary>
	/// Set the Invert SERDES RX Input bit
	/// </summary>
	/// <param name="link">Link to set</param>
	/// <param name="invert">Whether to invert</param>
	void SetInvertSERDESRXInput(DTC_Link_ID link, bool invert);
	/// <summary>
	/// Read the Invert SERDES TX Output bit
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Whether the Invert SERDES TX Output bit is set</returns>
	bool ReadInvertSERDESTXOutput(DTC_Link_ID link);
	/// <summary>
	/// Set the Invert SERDES TX Output bit
	/// </summary>
	/// <param name="link">Link to set</param>
	/// <param name="invert">Whether to invert</param>
	void SetInvertSERDESTXOutput(DTC_Link_ID link, bool invert);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESSerialInversionEnable();

	// Jitter Attenuator CSR Register
	/// <summary>
	/// Read the value of the Jitter Attenuator Select
	/// </summary>
	/// <returns>Jitter Attenuator Select value</returns>
	std::bitset<2> ReadJitterAttenuatorSelect();
	/// <summary>
	/// Set the Jitter Attenuator Select bits
	/// </summary>
	/// <param name="data">Value to set</param>
	void SetJitterAttenuatorSelect(std::bitset<2> data);
	/// <summary>
	/// Read the Jitter Attenuator Reset bit
	/// </summary>
	/// <returns>Value of the Jitter Attenuator Reset bit</returns>
	bool ReadJitterAttenuatorReset();
	/// <summary>
	/// Reset the Jitter Attenuator
	/// </summary>
	void ResetJitterAttenuator();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatJitterAttenuatorCSR();

	// EVB SERDES PRBS Control / Status Register
	/// <summary>
	/// Determine if an error was detected in the EVB SERDES PRBS module
	/// </summary>
	/// <returns>True if error condition was detected, false otherwise</returns>
	bool ReadEVBSERDESPRBSErrorFlag();
	/// <summary>
	/// Read the EVB SERDES TX PRBS Select
	/// </summary>
	/// <returns>Value of the EVB SERDES TX PRBS Select</returns>
	uint8_t ReadEVBSERDESTXPRBSSEL();
	/// <summary>
	/// Set the EVB SERDES TX PRBS Select
	/// </summary>
	/// <param name="byte">Value of the EVB SERDES TX PRBS Select</param>
	void SetEVBSERDESTXPRBSSEL(uint8_t byte);
	/// <summary>
	/// Read the EVB SERDES RX PRBS Select
	/// </summary>
	/// <returns>Value of the EVB SERDES RX PRBS Select</returns>
	uint8_t ReadEVBSERDESRXPRBSSEL();
	/// <summary>
	/// Set the EVB SERDES RX PRBS Select
	/// </summary>
	/// <param name="byte">Value of the EVB SERDES RX PRBS Select</param>
	void SetEVBSERDESRXPRBSSEL(uint8_t byte);
	/// <summary>
	/// Read the state of the EVB SERDES PRBS Force Error bit
	/// </summary>
	/// <returns>True if the EVB SERDES PRBS Force Error bit is high</returns>
	bool ReadEVBSERDESPRBSForceError();
	/// <summary>
	/// Set the EVB SERDES PRBS Force Error bit
	/// </summary>
	/// <param name="flag">New value for the EVB SERDES PRBS Reset bit</param>
	void SetEVBSERDESPRBSForceError(bool flag);
	/// <summary>
	/// Toggle the EVB SERDES PRBS Force Error bit (make it true if false, false if true)
	/// </summary>
	void ToggleEVBSERDESPRBSForceError();
	/// <summary>
	/// Read the state of the EVB SERDES PRBS Reset bit
	/// </summary>
	/// <returns>True if the EVB SERDES PRBS Reset bit is high</returns>
	bool ReadEVBSERDESPRBSReset();
	/// <summary>
	/// Set the EVB SERDES PRBS Reset bit
	/// </summary>
	/// <param name="flag">New value for the EVB SERDES PRBS Reset bit</param>
	void SetEVBSERDESPRBSReset(bool flag);
	/// <summary>
	/// Toggle the EVB SERDES PRBS Reset bit (make it true if false, false if true)
	/// </summary>
	void ToggleEVBSERDESPRBSReset();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatEVBSERDESPRBSControl();

	// Missed CFO Packet Count Registers
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 0
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 0</returns>
	uint32_t ReadMissedCFOPacketCountRing0();
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 1
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 1</returns>
	uint32_t ReadMissedCFOPacketCountRing1();
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 2
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 2</returns>
	uint32_t ReadMissedCFOPacketCountRing2();
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 3
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 3</returns>
	uint32_t ReadMissedCFOPacketCountRing3();
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 4
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 4</returns>
	uint32_t ReadMissedCFOPacketCountRing4();
	/// <summary>
	/// Reads the current value of the Missed CFO Packet Counter for Link 5
	/// </summary>
	/// <returns>the current value of the Missed CFO Packet Counter for Link 5</returns>
	uint32_t ReadMissedCFOPacketCountRing5();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 0
	/// </summary>
	void ClearMissedCFOPacketCountRing0();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 1
	/// </summary>
	void ClearMissedCFOPacketCountRing1();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 2
	/// </summary>
	void ClearMissedCFOPacketCountRing2();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 3
	/// </summary>
	void ClearMissedCFOPacketCountRing3();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 4
	/// </summary>
	void ClearMissedCFOPacketCountRing4();
	/// <summary>
	/// Clears the Missed CFO Packet Count for Link 5
	/// </summary>
	void ClearMissedCFOPacketCountRing5();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing0();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing1();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing2();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing3();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing4();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatMissedCFOPacketCountRing5();

	// Local Fragment Drop Count
	/// <summary>
	/// Reads the current value of the Local Fragment Drop Counter
	/// </summary>
	/// <returns>The number of fragments dropped by the DTC</returns>
	uint32_t ReadLocalFragmentDropCount();
	/// <summary>
	/// Clears the Local Fragment Drop Counter
	/// </summary>
	void ClearLocalFragmentDropCount();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatLocalFragmentDropCount();

	// Event Builder Error Register
	/// <summary>
	/// Read the Event Builder SubEvent Receiver Flags Buffer Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_SubEventReceiverFlagsBufferError();
	/// <summary>
	/// Read the Event Builder Ethernet Input FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_EthernetInputFIFOFull();
	/// <summary>
	/// Read the Event Builder Link Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_LinkError();
	/// <summary>
	/// Read the Event Builder TX Packet Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_TXPacketError();
	/// <summary>
	/// Read the Event Builder Local Data Pointer FIFO Queue Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_LocalDataPointerFIFOQueueError();
	/// <summary>
	/// Read the Event Builder Transmit DMA Byte Count FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadEventBuilder_TransmitDMAByteCountFIFOFull();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatEventBuilderErrorRegister();

	// SERDES VFIFO Error Register
	/// <summary>
	/// Read the SERDES VFIFO Egress FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_EgressFIFOFull();
	/// <summary>
	/// Read the SERDES VFIFO Ingress FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_IngressFIFOFull();
	/// <summary>
	/// Read the SERDES VFIFO Event Byte Count Total Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_EventByteCountTotalError();
	/// <summary>
	/// Read the SERDES VFIFO Last Word Written Timeout Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_LastWordWrittenTimeoutError();
	/// <summary>
	/// Read the SERDES VFIFO Fragment Count Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_FragmentCountError();
	/// <summary>
	/// Read the SERDES VFIFO DDR Full Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSERDESVFIFO_DDRFullError();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSERDESVFIFOError();

	// PCI VFIFO Error Register
	/// <summary>
	/// Read the PCI VIFO DDR Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_DDRFull();
	/// <summary>
	/// Read the PCI VIFO Memmory Mapped Write Complete FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_MemoryMappedWriteCompleteFIFOFull();
	/// <summary>
	/// Read the PCI VIFO PCI Write Event FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_PCIWriteEventFIFOFull();
	/// <summary>
	/// Read the PCI VIFO Local Data Pointer FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_LocalDataPointerFIFOFull();
	/// <summary>
	/// Read the PCI VIFO Egress FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_EgressFIFOFull();
	/// <summary>
	/// Read the PCI VIFO RX Buffer Select FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_RXBufferSelectFIFOFull();
	/// <summary>
	/// Read the PCI VIFO Ingress FIFO Full bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_IngressFIFOFull();
	/// <summary>
	/// Read the PCI VIFO Event Byte Count Total Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadPCIVFIFO_EventByteCountTotalError();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatPCIVFIFOError();

	// ROC Link Error Registers
	/// <summary>
	/// Read the Receive Data Request Sync Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_ROCDataRequestSyncError(DTC_Link_ID link);
	/// <summary>
	/// Read the Receive RX Packet Count Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_RXPacketCountError(DTC_Link_ID link);
	/// <summary>
	/// Read the Receive RX Packet Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_RXPacketError(DTC_Link_ID link);
	/// <summary>
	/// Read the Receive RX Packet CRC Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_RXPacketCRCError(DTC_Link_ID link);
	/// <summary>
	/// Read the Receive Data Pending Timeout Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_DataPendingTimeoutError(DTC_Link_ID link);
	/// <summary>
	/// Read the Receive Data Packet Count Error bit for the given link
	/// </summary>
	/// <param name="link">Link to read</param>
	/// <returns>Value of the bit</returns>
	bool ReadROCLink_ReceiveDataPacketCountError(DTC_Link_ID link);
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink0Error();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink1Error();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink2Error();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink3Error();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink4Error();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatRocLink5Error();

	// CFO Link Error Register
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatCFOLinkError();

	// Link Mux Error Register
	/// <summary>
	/// Read the DCS Mux Decode Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadDCSMuxDecodeError();
	/// <summary>
	/// Read the Data Mux Decode Error bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadDataMuxDecodeError();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatLinkMuxError();

	// Firefly CSR Register
	/// <summary>
	/// Read the TXRX Firefly Present bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXRXFireflyPresent();
	/// <summary>
	/// Read the RX Firefly Present bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadRXFireflyPresent();
	/// <summary>
	/// Read the TX Firefly Present bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXFireflyPresent();
	/// <summary>
	/// Read the TXRX Firefly Interrupt bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXRXFireflyInterrupt();
	/// <summary>
	/// Read the RX Firefly Interrupt bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadRXFireflyInterrupt();
	/// <summary>
	/// Read the TX Firefly Interrupt bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXFireflyInterrupt();
	/// <summary>
	/// Read the TXRX Firefly Select bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXRXFireflySelect();
	/// <summary>
	/// Set the TXRX Firefly Select bit
	/// </summary>
	/// <param name="select">Value to write</param>
	void SetTXRXFireflySelect(bool select);
	/// <summary>
	/// Read the TX Firefly Select bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadTXFireflySelect();
	/// <summary>
	/// Set the TX Firefly Select bit
	/// </summary>
	/// <param name="select">Value to write</param>
	void SetTXFireflySelect(bool select);
	/// <summary>
	/// Read the RX Firefly Select bit
	/// </summary>
	/// <returns>Value of bit</returns>
	bool ReadRXFireflySelect();
	/// <summary>
	/// Set the RX Firefly Select bit
	/// </summary>
	/// <param name="select">Value to write</param>
	void SetRXFireflySelect(bool select);
	/// <summary>
	/// Read the TXRX Firefly Reset bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadResetTXRXFirefly();
	void ResetTXRXFirefly();  ///< Reset the TXRX Firefly
	/// <summary>
	/// Read the TX Firefly Reset bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadResetTXFirefly();
	void ResetTXFirefly();  ///< Reset the TX Firefly
	/// <summary>
	/// Read the RX Firefly Reset bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadResetRXFirefly();
	void ResetRXFirefly();  ///< Reset the RX Firefly
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFireflyCSR();

	// SFP Control Status Register
	/// <summary>
	/// Read the SFP Present bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSFPPresent();
	/// <summary>
	/// Read the SFP LOS bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSFPLOS();
	/// <summary>
	/// Read the SFP TX Fault bit
	/// </summary>
	/// <returns>Value of the bit</returns>
	bool ReadSFPTXFault();
	/// <summary>
	/// Set the SFP Rate Select bit high
	/// </summary>
	void EnableSFPRateSelect();
	/// <summary>
	/// Set the SFP Rate Select bit low
	/// </summary>
	void DisableSFPRateSelect();
	/// <summary>
	/// Read the value of the SFP Rate Select bit
	/// </summary>
	/// <returns>The value of the SFP Rate Select bit</returns>
	bool ReadSFPRateSelect();
	/// <summary>
	/// Disable SFP TX
	/// </summary>
	void DisableSFPTX();
	/// <summary>
	/// Enable SFP TX
	/// </summary>
	void EnableSFPTX();
	/// <summary>
	/// Read the SFP TX Disable bit
	/// </summary>
	/// <returns>Value of the SFP TX Disable bit</returns>
	bool ReadSFPTXDisable();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatSFPControlStatus();

	// FPGA PROM Program Data Register

	// FPGA PROM Program Status Register
	/// <summary>
	/// Read the full bit on the FPGA PROM FIFO
	/// </summary>
	/// <returns>the full bit on the FPGA PROM FIFO</returns>
	bool ReadFPGAPROMProgramFIFOFull();
	/// <summary>
	/// Read whether the FPGA PROM is ready for data
	/// </summary>
	/// <returns>whether the FPGA PROM is ready for data</returns>
	bool ReadFPGAPROMReady();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGAPROMProgramStatus();

	// FPGA Core Access Register
	/// <summary>
	/// Performs the chants necessary to reload the DTC firmware
	/// </summary>
	void ReloadFPGAFirmware();
	/// <summary>
	/// Read the FPGA Core Access FIFO Full bit
	/// </summary>
	/// <returns>Whether the FPGA Core Access FIFO is full</returns>
	bool ReadFPGACoreAccessFIFOFull();
	/// <summary>
	/// Read the FPGA Core Access FIFO Empty bit
	/// </summary>
	/// <returns>Whether the FPGA Core Access FIFO is empty</returns>
	bool ReadFPGACoreAccessFIFOEmpty();
	/// <summary>
	/// Formats the register's current value for register dumps
	/// </summary>
	/// <returns>DTC_RegisterFormatter object containing register information</returns>
	DTC_RegisterFormatter FormatFPGACoreAccess();

	// Event Mode Lookup Table
	/// <summary>
	/// Set all event mode words to the given value
	/// </summary>
	/// <param name="data">Value for all event mode words</param>
	void SetAllEventModeWords(uint32_t data);
	/// <summary>
	/// Set a given event mode word
	/// </summary>
	/// <param name="which">Word index to write</param>
	/// <param name="data">Data for word</param>
	void SetEventModeWord(uint8_t which, uint32_t data);
	/// <summary>
	/// Read an event mode word from the Event Mode lookup table
	/// </summary>
	/// <param name="which">Word index to read</param>
	/// <returns>Value of the given event mode word</returns>
	uint32_t ReadEventModeWord(uint8_t which);

	// Oscillator Programming (DDR and SERDES)
	/// <summary>
	/// Set the given oscillator to the given frequency, calculating a new program in the process.
	/// </summary>
	/// <param name="oscillator">Oscillator to program, either DDR or SERDES</param>
	/// <param name="targetFrequency">New frequency to program, in Hz</param>
	/// <returns>Whether the oscillator was changed (Will not reset if already set to desired frequency)</returns>
	bool SetNewOscillatorFrequency(DTC_OscillatorType oscillator, double targetFrequency);
	/// <summary>
	/// Get the DTC's idea of the current frequency of the specified oscillator
	/// </summary>
	/// <param name="oscillator">Oscillator to program, either DDR or SERDES</param>
	/// <returns>Current frequency of oscillator, in Hz</returns>
	double ReadCurrentFrequency(DTC_OscillatorType oscillator);
	/// <summary>
	/// Read the current RFREQ and dividers of the given oscillator clock
	/// </summary>
	/// <param name="oscillator">Oscillator to program, either DDR or SERDES</param>
	/// <returns>64-bit integer contianing current oscillator program</returns>
	uint64_t ReadCurrentProgram(DTC_OscillatorType oscillator);
	/// <summary>
	/// Write the current frequency, in Hz to the frequency register
	/// </summary>
	/// <param name="freq">Frequency of oscillator</param>
	/// <param name="oscillator">Oscillator to program, either DDR or SERDES</param>
	void WriteCurrentFrequency(double freq, DTC_OscillatorType oscillator);
	/// <summary>
	/// Writes a program for the given oscillator crystal. This function should be paired with a call to
	/// WriteCurrentFrequency so that subsequent programming attempts work as expected.
	/// </summary>
	/// <param name="program">64-bit integer with new RFREQ and dividers</param>
	/// <param name="oscillator">Oscillator to program, either DDR or SERDES</param>
	void WriteCurrentProgram(uint64_t program, DTC_OscillatorType oscillator);

private:
	void WriteRegister_(uint32_t data, const DTC_Register& address);
	uint32_t ReadRegister_(const DTC_Register& address);

	int DecodeHighSpeedDivider_(int input);
	int DecodeOutputDivider_(int input) { return input + 1; }
	double DecodeRFREQ_(uint64_t input) { return input / 268435456.0; }
	int EncodeHighSpeedDivider_(int input);
	int EncodeOutputDivider_(int input);
	uint64_t EncodeRFREQ_(double input) { return static_cast<uint64_t>(input * 268435456) & 0x3FFFFFFFFF; }
	uint64_t CalculateFrequencyForProgramming_(double targetFrequency, double currentFrequency,
											   uint64_t currentProgram);
	uint64_t ReadSERDESOscillatorParameters_();
	uint64_t ReadTimingOscillatorParameters_();
	uint64_t ReadDDROscillatorParameters_();
	void SetSERDESOscillatorParameters_(uint64_t program);
	void SetTimingOscillatorParameters_(uint64_t program);
	void SetDDROscillatorParameters_(uint64_t program);

	bool WaitForLinkReady_(DTC_Link_ID const& link, size_t interval, double timeout = 2.0 /*seconds*/);

protected:
	mu2edev device_;              ///< Device handle
	DTC_SimMode simMode_;         ///< Simulation mode
	bool usingDetectorEmulator_{false};  ///< Whether Detector Emulation mode is enabled
	uint16_t dmaSize_;            ///< Size of DMAs, in bytes (default 32k)
	int formatterWidth_;          ///< Description field width, in characters

	/// <summary>
	/// Functions needed to print regular register map
	/// </summary>
	const std::vector<std::function<DTC_RegisterFormatter()>> formattedDumpFunctions_{
		[this]() { return this->FormatDesignVersion(); },
		[this]() { return this->FormatDesignDate(); },
		[this]() { return this->FormatDesignStatus(); },
		[this]() { return this->FormatVivadoVersion(); },
		[this]() { return this->FormatFPGATemperature(); },
		[this]() { return this->FormatFPGAVCCAUX(); },
		[this]() { return this->FormatFPGAVCCBRAM(); },
		[this]() { return this->FormatFPGAAlarms(); },
		[this]() { return this->FormatDTCControl(); },
		[this]() { return this->FormatDMATransferLength(); },
		[this]() { return this->FormatSERDESLoopbackEnable(); },
		[this]() { return this->FormatClockOscillatorStatus(); },
		[this]() { return this->FormatROCEmulationEnable(); },
		[this]() { return this->FormatRingEnable(); },
		[this]() { return this->FormatSERDESReset(); },
		[this]() { return this->FormatSERDESRXDisparityError(); },
		[this]() { return this->FormatSERDESRXCharacterNotInTableError(); },
		[this]() { return this->FormatSERDESUnlockError(); },
		[this]() { return this->FormatSERDESPLLLocked(); },
		[this]() { return this->FormatSERDESPLLPowerDown(); },
		[this]() { return this->FormatSERDESRXStatus(); },
		[this]() { return this->FormatSERDESResetDone(); },
		[this]() { return this->FormatRXCDRLockStatus(); },
		[this]() { return this->FormatDMATimeoutPreset(); },
		[this]() { return this->FormatROCReplyTimeout(); },
		[this]() { return this->FormatROCReplyTimeoutError(); },
		[this]() { return this->FormatRingPacketLength(); },
		[this]() { return this->FormatEVBLocalParitionIDMACIndex(); },
		[this]() { return this->FormatEVBNumberOfDestinationNodes(); },
		[this]() { return this->FormatTimingSERDESOscillatorFrequency(); },
		[this]() { return this->FormatMainBoardSERDESOscillatorFrequency(); },
		[this]() { return this->FormatSERDESOscillatorControl(); },
		[this]() { return this->FormatSERDESOscillatorParameterLow(); },
		[this]() { return this->FormatSERDESOscillatorParameterHigh(); },
		[this]() { return this->FormatDDROscillatorFrequency(); },
		[this]() { return this->FormatDDROscillatorControl(); },
		[this]() { return this->FormatDDROscillatorParameterLow(); },
		[this]() { return this->FormatDDROscillatorParameterHigh(); },
		[this]() { return this->FormatTimestampPreset0(); },
		[this]() { return this->FormatTimestampPreset1(); },
		[this]() { return this->FormatDataPendingTimer(); },
		[this]() { return this->FormatNUMROCs(); },
		[this]() { return this->FormatFIFOFullErrorFlag0(); },
		[this]() { return this->FormatFIFOFullErrorFlag1(); },
		[this]() { return this->FormatFIFOFullErrorFlag2(); },
		[this]() { return this->FormatReceivePacketError(); },
		[this]() { return this->FormatCFOEmulationTimestampLow(); },
		[this]() { return this->FormatCFOEmulationTimestampHigh(); },
		[this]() { return this->FormatCFOEmulationRequestInterval(); },
		[this]() { return this->FormatCFOEmulationNumRequests(); },
		[this]() { return this->FormatCFOEmulationNumPacketsLink01(); },
		[this]() { return this->FormatCFOEmulationNumPacketsLink23(); },
		[this]() { return this->FormatCFOEmulationNumPacketsLink45(); },
		[this]() { return this->FormatCFOEmulationNumNullHeartbeats(); },
		[this]() { return this->FormatCFOEmulationModeBytes03(); },
		[this]() { return this->FormatCFOEmulationModeBytes45(); },
		[this]() { return this->FormatCFOEmulationDebugPacketType(); },
		[this]() { return this->FormatRXPacketCountErrorFlags(); },
		[this]() { return this->FormatDetectorEmulationDMACount(); },
		[this]() { return this->FormatDetectorEmulationDMADelayCount(); },
		[this]() { return this->FormatDetectorEmulationControl0(); },
		[this]() { return this->FormatDetectorEmulationControl1(); },
		[this]() { return this->FormatDDRDataLocalStartAddress(); },
		[this]() { return this->FormatDDRDataLocalEndAddress(); },
		[this]() { return this->FormatROCDRPSyncError(); },
		[this]() { return this->FormatEthernetPayloadSize(); },
		[this]() { return this->FormatFireflyTXIICControl(); },
		[this]() { return this->FormatFireflyTXIICParameterLow(); },
		[this]() { return this->FormatFireflyTXIICParameterHigh(); },
		[this]() { return this->FormatFireflyRXIICControl(); },
		[this]() { return this->FormatFireflyRXIICParameterLow(); },
		[this]() { return this->FormatFireflyRXIICParameterHigh(); },
		[this]() { return this->FormatFireflyTXRXIICControl(); },
		[this]() { return this->FormatFireflyTXRXIICParameterLow(); },
		[this]() { return this->FormatFireflyTXRXIICParameterHigh(); },
		[this]() { return this->FormatDDRLinkBufferFullFlagsLow(); },
		[this]() { return this->FormatDDRLinkBufferFullFlagsHigh(); },
		[this]() { return this->FormatDDRLinkBufferFullErrorFlagsLow(); },
		[this]() { return this->FormatDDRLinkBufferFullErrorFlagsHigh(); },
		[this]() { return this->FormatDDRLinkBufferEmptyFlagsLow(); },
		[this]() { return this->FormatDDRLinkBufferEmptyFlagsHigh(); },
		[this]() { return this->FormatDDRLinkBufferHalfFullFlagsLow(); },
		[this]() { return this->FormatDDRLinkBufferHalfFullFlagsHigh(); },
		[this]() { return this->FormatDDREventBuilderBufferFullFlagsLow(); },
		[this]() { return this->FormatDDREventBuilderBufferFullFlagsHigh(); },
		[this]() { return this->FormatDDREventBuilderBufferFullErrorFlagsLow(); },
		[this]() { return this->FormatDDREventBuilderBufferFullErrorFlagsHigh(); },
		[this]() { return this->FormatDDREventBuilderBufferEmptyFlagsLow(); },
		[this]() { return this->FormatDDREventBuilderBufferEmptyFlagsHigh(); },
		[this]() { return this->FormatDDREventBuilderBufferHalfFullFlagsLow(); },
		[this]() { return this->FormatDDREventBuilderBufferHalfFullFlagsHigh(); },
		[this]() { return this->FormatSERDESSerialInversionEnable(); },
		[this]() { return this->FormatJitterAttenuatorCSR(); },
		[this]() { return this->FormatEVBSERDESPRBSControl(); },
		[this]() { return this->FormatMissedCFOPacketCountRing0(); },
		[this]() { return this->FormatMissedCFOPacketCountRing1(); },
		[this]() { return this->FormatMissedCFOPacketCountRing2(); },
		[this]() { return this->FormatMissedCFOPacketCountRing3(); },
		[this]() { return this->FormatMissedCFOPacketCountRing4(); },
		[this]() { return this->FormatMissedCFOPacketCountRing5(); },
		[this]() { return this->FormatLocalFragmentDropCount(); },
		[this]() { return this->FormatEventBuilderErrorRegister(); },
		[this]() { return this->FormatSERDESVFIFOError(); },
		[this]() { return this->FormatPCIVFIFOError(); },
		[this]() { return this->FormatRocLink0Error(); },
		[this]() { return this->FormatRocLink1Error(); },
		[this]() { return this->FormatRocLink2Error(); },
		[this]() { return this->FormatRocLink3Error(); },
		[this]() { return this->FormatRocLink4Error(); },
		[this]() { return this->FormatRocLink5Error(); },
		[this]() { return this->FormatCFOLinkError(); },
		[this]() { return this->FormatLinkMuxError(); },
		[this]() { return this->FormatFireflyCSR(); },
		[this]() { return this->FormatSFPControlStatus(); },
		[this]() { return this->FormatFPGAPROMProgramStatus(); },
		[this]() { return this->FormatFPGACoreAccess(); }};

	/// <summary>
	/// Dump Byte/Packet Counter Registers
	/// </summary>
	const std::vector<std::function<DTC_RegisterFormatter()>> formattedCounterFunctions_{
		[this]() { return this->FormatReceiveByteCountRing0(); },
		[this]() { return this->FormatReceiveByteCountRing1(); },
		[this]() { return this->FormatReceiveByteCountRing2(); },
		[this]() { return this->FormatReceiveByteCountRing3(); },
		[this]() { return this->FormatReceiveByteCountRing4(); },
		[this]() { return this->FormatReceiveByteCountRing5(); },
		[this]() { return this->FormatReceiveByteCountCFO(); },
		[this]() { return this->FormatReceiveByteCountEVB(); },
		[this]() { return this->FormatReceivePacketCountRing0(); },
		[this]() { return this->FormatReceivePacketCountRing1(); },
		[this]() { return this->FormatReceivePacketCountRing2(); },
		[this]() { return this->FormatReceivePacketCountRing3(); },
		[this]() { return this->FormatReceivePacketCountRing4(); },
		[this]() { return this->FormatReceivePacketCountRing5(); },
		[this]() { return this->FormatReceivePacketCountCFO(); },
		[this]() { return this->FormatReceivePacketCountEVB(); },
		[this]() { return this->FormatTramsitByteCountRing0(); },
		[this]() { return this->FormatTramsitByteCountRing1(); },
		[this]() { return this->FormatTramsitByteCountRing2(); },
		[this]() { return this->FormatTramsitByteCountRing3(); },
		[this]() { return this->FormatTramsitByteCountRing4(); },
		[this]() { return this->FormatTramsitByteCountRing5(); },
		[this]() { return this->FormatTramsitByteCountCFO(); },
		[this]() { return this->FormatTramsitByteCountEVB(); },
		[this]() { return this->FormatTransmitPacketCountRing0(); },
		[this]() { return this->FormatTransmitPacketCountRing1(); },
		[this]() { return this->FormatTransmitPacketCountRing2(); },
		[this]() { return this->FormatTransmitPacketCountRing3(); },
		[this]() { return this->FormatTransmitPacketCountRing4(); },
		[this]() { return this->FormatTransmitPacketCountRing5(); },
		[this]() { return this->FormatTransmitPacketCountCFO(); },
		[this]() { return this->FormatTransmitPacketCountEVB(); }};
};
}  // namespace DTCLib

#endif  // DTC_REGISTERS_H
