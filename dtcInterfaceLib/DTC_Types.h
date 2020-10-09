#ifndef DTC_TYPES_H
#define DTC_TYPES_H

#include <bitset>   // std::bitset
#include <cstdint>  // uint8_t, uint16_t
#include <iomanip>
#include <vector>  // std::vector
#include "TRACE/tracemf.h"

namespace DTCLib {

typedef uint16_t roc_address_t;
typedef uint16_t roc_data_t;

enum DTC_Subsystem : uint8_t
{
	DTC_Subsystem_Tracker = 0,
	DTC_Subsystem_Calorimeter = 1,
	DTC_Subsystem_CRV = 2,
	DTC_Subsystem_Other = 3,
	DTC_Subsystem_STM = 4,
	DTC_Subsystem_ExtMon = 5,
};

enum DTC_Link_ID : uint8_t
{
	DTC_Link_0 = 0,
	DTC_Link_1 = 1,
	DTC_Link_2 = 2,
	DTC_Link_3 = 3,
	DTC_Link_4 = 4,
	DTC_Link_5 = 5,
	DTC_Link_CFO = 6,
	DTC_Link_EVB = 7,
	DTC_Link_Unused,
};

static const std::vector<DTC_Link_ID> DTC_Links{DTC_Link_0, DTC_Link_1, DTC_Link_2, DTC_Link_3, DTC_Link_4, DTC_Link_5};

enum DTC_OscillatorType
{
	DTC_OscillatorType_SERDES,
	DTC_OscillatorType_DDR,
	DTC_OscillatorType_Timing,
};

enum DTC_SerdesClockSpeed
{
	DTC_SerdesClockSpeed_25Gbps,
	DTC_SerdesClockSpeed_3125Gbps,
	DTC_SerdesClockSpeed_48Gbps,
	DTC_SerdesClockSpeed_Unknown
};

enum DTC_DebugType
{
	DTC_DebugType_SpecialSequence = 0,
	DTC_DebugType_ExternalSerial = 1,
	DTC_DebugType_ExternalSerialWithReset = 2,
	DTC_DebugType_RAMTest = 3,
	DTC_DebugType_DDRTest = 4,
	DTC_DebugType_Invalid = 5,
};

/// <summary>
/// The DTC_DebugTypeConverter converts a DTC_DebugType enumeration value to string or JSON representation
/// </summary>
struct DTC_DebugTypeConverter
{
	DTC_DebugType type_;  ///< DTC_DebugType to convert

	/// <summary>
	/// Construct a DTC_DebugTypeConverter instance using the given DTC_DebugType
	/// </summary>
	/// <param name="type">DTC_DebugType to convert</param>
	explicit DTC_DebugTypeConverter(DTC_DebugType type)
		: type_(type) {}

	/// <summary>
	/// Convert the DTC_DebugType to its string representation
	/// </summary>
	/// <returns>String representation of DTC_DebugType</returns>
	std::string toString() const
	{
		switch (type_)
		{
			case DTC_DebugType_SpecialSequence:
				return "Special Sequence";
			case DTC_DebugType_ExternalSerial:
				return "External Serial";
			case DTC_DebugType_ExternalSerialWithReset:
				return "External Serial with FIFO Reset";
			case DTC_DebugType_RAMTest:
				return "FPGA SRAM Error Checking";
			case DTC_DebugType_DDRTest:
				return "DDR3 Memory Error Checking";
			case DTC_DebugType_Invalid:
				return "INVALID!!!";
		}
		return "Unknown";
	}

	/// <summary>
	/// Write a DTC_DebugTypeConverter in JSON format to the given stream
	/// </summary>
	/// <param name="stream">Stream to write</param>
	/// <param name="type">DTC_DebugTypeConverter to serialize</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_DebugTypeConverter& type)
	{
		stream << "\"DTC_DebugType\":\"" << type.toString() << "\"";
		return stream;
	}
};

enum DTC_RXBufferStatus
{
	DTC_RXBufferStatus_Nominal = 0,
	DTC_RXBufferStatus_BufferEmpty = 1,
	DTC_RXBufferStatus_BufferFull = 2,
	DTC_RXBufferStatus_Underflow = 5,
	DTC_RXBufferStatus_Overflow = 6,
	DTC_RXBufferStatus_Unknown = 0x10,
};

/// <summary>
/// The DTC_RXBufferStatusConverter converts a DTC_RXBufferStatus enumeration value to string or JSON representation
/// </summary>
struct DTC_RXBufferStatusConverter
{
	DTC_RXBufferStatus status_;  ///< DTC_RXBufferStatus to convert

	/// <summary>
	/// Construct a DTC_RXBufferStatusConverter instance using the given DTC_RXBufferStatus
	/// </summary>
	/// <param name="status">DTC_RXBufferStatus to convert</param>
	explicit DTC_RXBufferStatusConverter(DTC_RXBufferStatus status)
		: status_(status) {}

	/// <summary>
	/// Convert the DTC_RXBufferStatus to its string representation
	/// </summary>
	/// <returns>String representation of DTC_RXBufferStatus</returns>
	std::string toString() const
	{
		switch (status_)
		{
			case DTC_RXBufferStatus_Nominal:
				return "Nominal";
			case DTC_RXBufferStatus_BufferEmpty:
				return "BufferEmpty";
			case DTC_RXBufferStatus_BufferFull:
				return "BufferFull";
			case DTC_RXBufferStatus_Overflow:
				return "Overflow";
			case DTC_RXBufferStatus_Underflow:
				return "Underflow";
			case DTC_RXBufferStatus_Unknown:
			default:
				return "Unknown";
		}
	}

	/// <summary>
	/// Write a DTC_RXBufferStatusConverter in JSON format to the given stream
	/// </summary>
	/// <param name="stream">Stream to write</param>
	/// <param name="status">DTC_RXBufferStatusConverter to serialize</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_RXBufferStatusConverter& status)
	{
		stream << "\"DTC_RXBufferStatus\":\"" << status.toString() << "\"";
		return stream;
	}
};

enum DTC_RXStatus
{
	DTC_RXStatus_DataOK = 0,
	DTC_RXStatus_SKPAdded = 1,
	DTC_RXStatus_SKPRemoved = 2,
	DTC_RXStatus_ReceiverDetected = 3,
	DTC_RXStatus_DecodeError = 4,
	DTC_RXStatus_ElasticOverflow = 5,
	DTC_RXStatus_ElasticUnderflow = 6,
	DTC_RXStatus_RXDisparityError = 7,
};

/// <summary>
/// The DTC_RXStatusConverter converts a DTC_RXStatus enumeration value to string or JSON representation
/// </summary>
struct DTC_RXStatusConverter
{
	DTC_RXStatus status_;  ///< DTC_RXStatus to convert

	/// <summary>
	/// Construct a DTC_RXStatusConverter instance using the given DTC_RXStatus
	/// </summary>
	/// <param name="status">DTC_RXStatus to convert</param>
	explicit DTC_RXStatusConverter(DTC_RXStatus status);

	/// <summary>
	/// Convert the DTC_RXStatus to its string representation
	/// </summary>
	/// <returns>String representation of DTC_RXStatus</returns>
	std::string toString() const
	{
		switch (status_)
		{
			case DTC_RXStatus_DataOK:
				return "DataOK";
			case DTC_RXStatus_SKPAdded:
				return "SKPAdded";
			case DTC_RXStatus_SKPRemoved:
				return "SKPRemoved";
			case DTC_RXStatus_ReceiverDetected:
				return "ReceiverDetected";
			case DTC_RXStatus_DecodeError:
				return "DecodeErr";
			case DTC_RXStatus_ElasticOverflow:
				return "ElasticOF";
			case DTC_RXStatus_ElasticUnderflow:
				return "ElasticUF";
			case DTC_RXStatus_RXDisparityError:
				return "RXDisparity";
		}
		return "Unknown";
	}

	/// <summary>
	/// Write a DTC_RXStatusConverter in JSON format to the given stream
	/// </summary>
	/// <param name="stream">Stream to write</param>
	/// <param name="status">DTC_RXStatusConverter to serialize</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_RXStatusConverter& status)
	{
		stream << "\"DTC_RXStatus\":\"" << status.toString() << "\"";
		return stream;
	}
};

enum DTC_SERDESLoopbackMode
{
	DTC_SERDESLoopbackMode_Disabled = 0,
	DTC_SERDESLoopbackMode_NearPCS = 1,
	DTC_SERDESLoopbackMode_NearPMA = 2,
	DTC_SERDESLoopbackMode_FarPMA = 4,
	DTC_SERDESLoopbackMode_FarPCS = 6,
};

/// <summary>
/// The DTC_SERDESLoopbackModeConverter converts a DTC_SERDESLoopbackMode enumeration value to string or JSON
/// representation
/// </summary>
struct DTC_SERDESLoopbackModeConverter
{
	DTC_SERDESLoopbackMode mode_;  ///< DTC_SERDESLoopbackMode to convert

	/// <summary>
	/// Construct a DTC_SERDESLoopbackModeConverter instance using the given DTC_SERDESLoopbackMode
	/// </summary>
	/// <param name="mode">DTC_SERDESLoopbackMode to convert</param>
	explicit DTC_SERDESLoopbackModeConverter(DTC_SERDESLoopbackMode mode)
		: mode_(mode) {}

	/// <summary>
	/// Convert the DTC_SERDESLoopbackMode to its string representation
	/// </summary>
	/// <returns>String representation of DTC_SERDESLoopbackMode</returns>
	std::string toString() const
	{
		switch (mode_)
		{
			case DTC_SERDESLoopbackMode_Disabled:
				return "Disabled";
			case DTC_SERDESLoopbackMode_NearPCS:
				return "NearPCS";
			case DTC_SERDESLoopbackMode_NearPMA:
				return "NearPMA";
			case DTC_SERDESLoopbackMode_FarPMA:
				return "FarPMA";
			case DTC_SERDESLoopbackMode_FarPCS:
				return "FarPCS";
		}
		return "Unknown";
	}

	/// <summary>
	/// Write a DTC_SERDESLoopbackModeConverter in JSON format to the given stream
	/// </summary>
	/// <param name="stream">Stream to write</param>
	/// <param name="mode">DTC_SERDESLoopbackModeConverter to serialize</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_SERDESLoopbackModeConverter& mode)
	{
		stream << "\"DTC_SERDESLoopbackMode\":\"" << mode.toString() << "\"";
		return stream;
	}
};

/// <summary>
/// The DTC_SimMode enumeration is used to control the behavior of the DTC class.
///
/// DTC_SimMode_Tracker, Calorimeter, CosmicVeto, Performance, and LargeFile activate the mu2esim DTC emulator
/// in the corresponding mode.
/// DTC_SimMode_Disabled does nothing to set up the DTC beyond basic initialization.
/// DTC_SimMode_NoCFO enables the DTC CFO Emulator to send ReadoutRequest and DataRequest packets.
/// DTC_SimMode_ROCEmulator enables the DTC ROC Emulator
/// DTC_SimMode_Loopback enables the SERDES loopback on the DTC
/// </summary>
enum DTC_SimMode
{
	DTC_SimMode_Disabled = 0,
	DTC_SimMode_Tracker = 1,
	DTC_SimMode_Calorimeter = 2,
	DTC_SimMode_CosmicVeto = 3,
	DTC_SimMode_NoCFO = 4,
	DTC_SimMode_ROCEmulator = 5,
	DTC_SimMode_Loopback = 6,
	DTC_SimMode_Performance = 7,
	DTC_SimMode_LargeFile = 8,
	DTC_SimMode_Timeout,
	DTC_SimMode_Invalid,
};

/// <summary>
/// The DTC_SimModeConverter converts a DTC_SimMode enumeration value to string or JSON representation
/// </summary>
struct DTC_SimModeConverter
{
	DTC_SimMode mode_;  ///< DTC_SimMode to convert to string

	/// <summary>
	/// Construct a DTC_SimModeConverter instance using the given DTC_SimMode
	/// </summary>
	/// <param name="mode">DTC_SimMode to convert</param>
	explicit DTC_SimModeConverter(DTC_SimMode mode)
		: mode_(mode) {}

	/// <summary>
	/// Parse a string and return the DTC_SimMode which corresponds to it
	///
	/// Will search for SimMode name (see DTC_SimModeConverter::toString(),), or integer value (i.e. 1 =
	/// DTC_SimMode_Tracker, see enumeration definition)
	/// </summary>
	/// <param name="s">String to parse</param>
	/// <returns>DTC_SimMode corresponding to string</returns>
	static DTC_SimMode ConvertToSimMode(std::string s);

	/// <summary>
	/// Convert the DTC_SimMode to its string representation
	/// </summary>
	/// <returns>String representation of DTC_SimMode</returns>
	std::string toString() const
	{
		switch (mode_)
		{
			case DTC_SimMode_Tracker:
				return "Tracker";
			case DTC_SimMode_Calorimeter:
				return "Calorimeter";
			case DTC_SimMode_CosmicVeto:
				return "CosmicVeto";
			case DTC_SimMode_NoCFO:
				return "NoCFO";
			case DTC_SimMode_ROCEmulator:
				return "ROCEmulator";
			case DTC_SimMode_Loopback:
				return "Loopback";
			case DTC_SimMode_Performance:
				return "Performance";
			case DTC_SimMode_LargeFile:
				return "LargeFile";
			case DTC_SimMode_Disabled:
			default:
				return "Disabled";
		}
	}

	/// <summary>
	/// Write a DTC_SimModeConverter in JSON format to the given stream
	/// </summary>
	/// <param name="stream">Stream to write</param>
	/// <param name="mode">DTC_SimModeConverter to serialize</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_SimModeConverter& mode)
	{
		stream << "\"DTC_SimMode\":\"" << mode.toString() << "\"";
		return stream;
	}
};

/// <summary>
/// A DTC_WrongVersionException is thrown when an attempt to initialize a DTC is made with a certain firmware version
/// expected, and the firmware does not match that version
/// </summary>
class DTC_WrongVersionException : public std::exception
{
public:
	/// <summary>
	/// A DTC_WrongVersionException is thrown when an attempt is made to construct a DTC packet with data that does not
	/// match the packet type
	/// </summary>
	/// <param name="expected">Expected firmware version string</param>
	/// <param name="encountered">Encountered firmware version string</param>
	DTC_WrongVersionException(std::string expected, std::string encountered)
		: what_("DTCwrongVersionException: Unexpected firmware version encountered: " + encountered + " != " + expected + " (expected)") {}
	/// <summary>
	/// Describe the exception
	/// </summary>
	/// <returns>String describing the exception</returns>
	const char* what() const throw()
	{
		return what_.c_str();
	}

private:
	std::string what_;
};

/// <summary>
/// A DTC_WrongPacketTypeException is thrown when an attempt to decode a DMA packet is made and the type in the DMA
/// header is different than the type of the packet expected
/// </summary>
class DTC_WrongPacketTypeException : public std::exception
{
public:
	/// <summary>
	/// A DTC_WrongPacketTypeException is thrown when an attempt is made to construct a DTC packet with data that does not
	/// match the packet type
	/// </summary>
	/// <param name="expected">Expected packet type</param>
	/// <param name="encountered">Encountered packet type</param>
	DTC_WrongPacketTypeException(int expected, int encountered)
		: what_("DTCWrongPacketTypeException: Unexpected packet type encountered: " + std::to_string(encountered) + " != " + std::to_string(expected) + " (expected)") {}
	/// <summary>
	/// Describe the exception
	/// </summary>
	/// <returns>String describing the exception</returns>
	const char* what() const throw()
	{
		return what_.c_str();
	}

private:
	std::string what_;
};

/// <summary>
/// A DTC_IOErrorException is thrown when the DTC is not communicating when communication is expected
/// </summary>
class DTC_IOErrorException : public std::exception
{
public:
	/// <summary>
	/// A DTC_IOErrorException is thrown when an attempt is made to read or write from the DTC, and an unexpected status
	/// results
	/// </summary>
	/// <param name="retcode">Return code from IO operation</param>
	DTC_IOErrorException(int retcode)
		: what_(std::string("DTCIOErrorException: Unable to communicate with the DTC: Error Code: ") + std::to_string(retcode)) {}
	/// <summary>
	/// Describe the exception
	/// </summary>
	/// <returns>String describing the exception</returns>
	const char* what() const throw()
	{
		return what_.c_str();
	}

private:
	std::string what_;
};

/// <summary>
/// A DTC_DataCorruptionException is thrown when corrupt data is detected coming from the DTC
/// </summary>
class DTC_DataCorruptionException : public std::exception
{
public:
	/// <summary>
	/// Describe the exception
	/// </summary>
	/// <returns>String describing the exception</returns>
	const char* what() const throw() { return "DTCDataCorruptionException: Corruption detected in data stream from DTC"; }
};

/// <summary>
/// The mu2e Timestamp is a 48-bit quantity. This class manages all the different ways it could be accessed.
/// </summary>
class DTC_Timestamp
{
	uint64_t timestamp_ : 48;

public:
	/// <summary>
	/// Default Constructor. Initializes timestamp to value 0
	/// </summary>
	DTC_Timestamp();
	/// <summary>
	/// Construct a timestamp using the given quad word
	/// </summary>
	/// <param name="timestamp">64-bit unsigned integer representing timestamp. Top 16 bits will be discarded</param>
	explicit DTC_Timestamp(const uint64_t timestamp);
	/// <summary>
	/// Construct a timestamp using the given low and high words
	/// </summary>
	/// <param name="timestampLow">Lower 32 bits of timestamp</param>
	/// <param name="timestampHigh">Upper 16 bits of timestamp</param>
	DTC_Timestamp(const uint32_t timestampLow, const uint16_t timestampHigh);
	/// <summary>
	/// Construct a DTC_Timestamp using the given byte array. Length of the array must be greater than 6 + offset!
	/// </summary>
	/// <param name="timeArr">Byte array to read timestamp from (i.e. DTC_DataPacket::GetData())</param>
	/// <param name="offset">Location of timestamp byte 0 in array (Default 0)</param>
	explicit DTC_Timestamp(const uint8_t* timeArr, int offset = 0);
	/// <summary>
	/// Construct a DTC_Timestamp using a std::bitset of the 48-bit timestamp
	/// </summary>
	/// <param name="timestamp">std::bitset containing timestamp</param>
	explicit DTC_Timestamp(const std::bitset<48> timestamp);
	/// <summary>
	/// Default copy constructor
	/// </summary>
	/// <param name="r">DTC_Timestamp to copy</param>
	DTC_Timestamp(const DTC_Timestamp& r) = default;
	/// <summary>
	/// Default move constructor
	/// </summary>
	/// <param name="r">DTC_Timestamp rvalue</param>
	DTC_Timestamp(DTC_Timestamp&& r) = default;

	/// <summary>
	/// DTC_Timestamp Default Destructor
	/// </summary>
	virtual ~DTC_Timestamp() = default;
	/// <summary>
	/// Default move assignment operator
	/// </summary>
	/// <param name="r">DTC_Timestamp rvalue</param>
	/// <returns>DTC_Timestamp reference</returns>
	DTC_Timestamp& operator=(DTC_Timestamp&& r) = default;
	/// <summary>
	/// Default copy assignment operator
	/// </summary>
	/// <param name="r">DTC_Timestamp to copy</param>
	/// <returns>DTC_Timestamp reference</returns>
	DTC_Timestamp& operator=(const DTC_Timestamp& r) = default;

	/// <summary>
	/// Compare two DTC_Timestamp instances
	/// </summary>
	/// <param name="r">Other timestamp</param>
	/// <returns>Result of comparison</returns>
	bool operator==(const DTC_Timestamp r) const { return r.GetTimestamp(true) == timestamp_; }

	/// <summary>
	/// Compare two DTC_Timestamp instances
	/// </summary>
	/// <param name="r">Other timestamp</param>
	/// <returns>Result of comparison</returns>
	bool operator!=(const DTC_Timestamp r) const { return r.GetTimestamp(true) != timestamp_; }

	/// <summary>
	/// Compare two DTC_Timestamp instances
	/// </summary>
	/// <param name="r">Other timestamp</param>
	/// <returns>Result of comparison</returns>
	bool operator<(const DTC_Timestamp r) { return r.GetTimestamp(true) > timestamp_; }

	/// <summary>
	/// Compare two DTC_Timestamp instances
	/// </summary>
	/// <param name="r">Other timestamp</param>
	/// <returns>Result of comparison</returns>
	bool operator<(const DTC_Timestamp r) const { return r.GetTimestamp(true) > timestamp_; }

	/// <summary>
	/// Add an integer to a timestamp instance
	/// </summary>
	/// <param name="r">Integer to add to timestamp</param>
	/// <returns>New timestamp with result</returns>
	DTC_Timestamp operator+(const int r) const { return DTC_Timestamp(r + timestamp_); }

	/// <summary>
	/// Set the timestamp using the given quad word
	/// </summary>
	/// <param name="timestamp">64-bit unsigned integer representing timestamp. Top 16 bits will be discarded</param>
	void SetTimestamp(uint64_t timestamp) { timestamp_ = timestamp & 0x0000FFFFFFFFFFFF; }

	/// <summary>
	/// Set the timestamp using the given low and high words
	/// </summary>
	/// <param name="timestampLow">Lower 32 bits of the timestamp</param>
	/// <param name="timestampHigh">Upper 16 bits of the timstamp</param>
	void SetTimestamp(uint32_t timestampLow, uint16_t timestampHigh);

	/// <summary>
	/// Returns the timstamp as a 48-bit std::bitset
	/// </summary>
	/// <returns>The timestamp as a 48-bit std::bitset</returns>
	std::bitset<48> GetTimestamp() const { return timestamp_; }

	/// <summary>
	/// Returns the timestamp as a 64-bit unsigned integer
	/// </summary>
	/// <param name="dummy">Whether to return a timestamp (used to distinguish signature)</param>
	/// <returns>Timestamp as a 64-bit unsigned integer</returns>
	uint64_t GetTimestamp(bool dummy) const
	{
		if (dummy)
		{
			return timestamp_;
		}
		return 0;
	}

	/// <summary>
	/// Copies the timestamp into the given byte array, starting at some offset.
	/// Size of input array MUST be larger than offset + 6
	/// </summary>
	/// <param name="timeArr">Byte array for output</param>
	/// <param name="offset">Target index of byte 0 of the timestamp</param>
	void GetTimestamp(const uint8_t* timeArr, int offset = 0) const;

	/// <summary>
	/// Convert the timestamp to a JSON representation
	/// </summary>
	/// <param name="arrayMode">(Default: false) If true, will create a JSON array of the 6 bytes. Otherwise, represents
	/// timestamp as a single number</param> <returns>JSON-formatted string containing timestamp</returns>
	std::string toJSON(bool arrayMode = false) const;

	/// <summary>
	/// Convert the 48-bit timestamp to the format used in the Packet format definitions.
	/// Byte 1 | Byte 0
	/// Byte 3 | Byte 2
	/// Byte 5 | Byte 4
	/// </summary>
	/// <returns>String representing timestamp in "packet format"</returns>
	std::string toPacketFormat() const;
};

/// <summary>
/// This class is used to decode the SERDES RX Disparity Error register
/// </summary>
class DTC_SERDESRXDisparityError
{
	std::bitset<2> data_;

public:
	/// <summary>
	/// Default Constructor
	/// </summary>
	DTC_SERDESRXDisparityError();
	/// <summary>
	/// Construct a DTC_SERDESRXDisparityError using the given bitset
	/// </summary>
	/// <param name="data">Bits to use for initialization</param>
	explicit DTC_SERDESRXDisparityError(std::bitset<2> data);
	/// <summary>
	/// Construct a DTC_SERDESRXDisparityError using the register value and a Link ID
	/// </summary>
	/// <param name="data">Register value to read error bits from</param>
	/// <param name="link">Link to read</param>
	DTC_SERDESRXDisparityError(uint32_t data, DTC_Link_ID link);
	/// <summary>
	/// Default Copy Constructor
	/// </summary>
	/// <param name="in">DTC_SERDESRXDisparityError to copy</param>
	DTC_SERDESRXDisparityError(const DTC_SERDESRXDisparityError& in) = default;
	/// <summary>
	/// Default Move Constructor
	/// </summary>
	/// <param name="in">DTC_SERDESRXDisparityError rvalue</param>
	DTC_SERDESRXDisparityError(DTC_SERDESRXDisparityError&& in) = default;

	/// <summary>
	/// Default Copy Assignment Operator
	/// </summary>
	/// <param name="in">DTC_SERDESRXDisparityError to copy</param>
	/// <returns>DTC_SERDESRXDisparityError Reference</returns>
	DTC_SERDESRXDisparityError& operator=(const DTC_SERDESRXDisparityError& in) = default;
	/// <summary>
	/// Default Move Assignment Operator
	/// </summary>
	/// <param name="in">DTC_SERDESRXDisparityError rvalue</param>
	/// <returns>DTC_SERDESRXDisparityError reference</returns>
	DTC_SERDESRXDisparityError& operator=(DTC_SERDESRXDisparityError&& in) = default;

	/// <summary>
	/// Sets the data bits
	/// </summary>
	/// <param name="data">Data bits to set</param>
	void SetData(std::bitset<2> data) { data_ = data; }

	/// <summary>
	/// Gets the data bitset
	/// </summary>
	/// <returns>Data bits</returns>
	std::bitset<2> GetData() const { return data_; }

	/// <summary>
	/// Get the integer representation of the error flags
	/// </summary>
	/// <param name="output">Whether to output data (signature different than bitset version)</param>
	/// <returns>Data as int</returns>
	int GetData(bool output) const
	{
		if (output) return static_cast<int>(data_.to_ulong());
		return 0;
	}

	/// <summary>
	/// Write the DTC_SERDESRXDisparityError to stream in JSON format.
	/// Note that the JSON represents an object, calling code should add a key if there's other objects to stream
	/// </summary>
	/// <param name="stream">Stream to write to</param>
	/// <param name="error">DTC_SERDESRXDisparityError to parse</param>
	/// <returns>Stream object for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, DTC_SERDESRXDisparityError error)
	{
		stream << "{\"low\":" << error.GetData()[0] << ",\"high\":" << error.GetData()[1] << "}";
		return stream;
	}
};

/// <summary>
/// This structure is used to decode the SERDES Character Not In Table Error register
/// </summary>
class DTC_CharacterNotInTableError
{
	std::bitset<2> data_;

public:
	/// <summary>
	/// Default Constructor
	/// Initializes data bits to 0,0
	/// </summary>
	DTC_CharacterNotInTableError();
	/// <summary>
	/// Constructor using data bits
	/// </summary>
	/// <param name="data">data bits for this instance</param>
	explicit DTC_CharacterNotInTableError(std::bitset<2> data);
	/// <summary>
	/// DTC_CharacterNotInTableError Constructor
	/// </summary>
	/// <param name="data">Register value</param>
	/// <param name="link">Specific link to read</param>
	DTC_CharacterNotInTableError(uint32_t data, DTC_Link_ID link);
	/// <summary>
	/// Default Copy Constructor
	/// </summary>
	/// <param name="in">DTC_CharacterNotInTableError to copy</param>
	DTC_CharacterNotInTableError(const DTC_CharacterNotInTableError& in) = default;
	/// <summary>
	/// Default Move Constructor
	/// </summary>
	/// <param name="in">DTC_CharacterNotInTableError rvalue</param>
	DTC_CharacterNotInTableError(DTC_CharacterNotInTableError&& in) = default;

	/// <summary>
	/// Default Copy Assignment Operator
	/// </summary>
	/// <param name="in">DTC_CharacterNotInTableError to copy</param>
	/// <returns>DTC_CharacterNotInTableError reference</returns>
	DTC_CharacterNotInTableError& operator=(const DTC_CharacterNotInTableError& in) = default;
	/// <summary>
	/// Default Move Assignment Operator
	/// </summary>
	/// <param name="in">DTC_CharacterNotInTableError rvalue</param>
	/// <returns>DTC_CharacterNotInTableError reference</returns>
	DTC_CharacterNotInTableError& operator=(DTC_CharacterNotInTableError&& in) = default;

	/// <summary>
	/// Sets the data bits
	/// </summary>
	/// <param name="data">Data bits to set</param>
	void SetData(std::bitset<2> data) { data_ = data; }

	/// <summary>
	/// Gets the data bitset
	/// </summary>
	/// <returns>Data bits</returns>
	std::bitset<2> GetData() const { return data_; }

	/// <summary>
	/// Get the integer representation of the error flags
	/// </summary>
	/// <param name="output">Whether to output data (signature different than bitset version)</param>
	/// <returns>Data as int</returns>
	int GetData(bool output) const
	{
		if (output) return static_cast<int>(data_.to_ulong());
		return 0;
	}

	/// <summary>
	/// Write the DTC_CharacterNotInTableError to stream in JSON format.
	/// Note that the JSON represents an object, calling code should add a key if there's other objects to stream
	/// </summary>
	/// <param name="stream">Stream to write to</param>
	/// <param name="error">DTC_CharacterNotInTableError to parse</param>
	/// <returns>Stream object for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, DTC_CharacterNotInTableError error)
	{
		stream << "{\"low\":" << error.GetData()[0] << ",\"high\":" << error.GetData()[1] << "}";
		return stream;
	}
};

/// <summary>
/// This structure is used to decode the LinkEnable register value
/// </summary>
struct DTC_LinkEnableMode
{
	bool TransmitEnable;  ///< Whether transmit is enabled on this link
	bool ReceiveEnable;   ///< Whether receive is enabled on this link
	bool TimingEnable;    ///< Whether timing is enabled on this link

	/// <summary>
	/// Default constructor. Sets all enable bits to true.
	/// </summary>
	DTC_LinkEnableMode()
		: TransmitEnable(true), ReceiveEnable(true), TimingEnable(true) {}

	/// <summary>
	/// Construct a DTC_LinkEnableMode instance with the given flags
	/// </summary>
	/// <param name="transmit">Enable TX</param>
	/// <param name="receive">Enable RX</param>
	/// <param name="timing">Enable CFO</param>
	DTC_LinkEnableMode(bool transmit, bool receive, bool timing)
		: TransmitEnable(transmit), ReceiveEnable(receive), TimingEnable(timing) {}

	/// <summary>
	/// Write the DTC_LinkEnableMode to stream in JSON format.
	/// Note that the JSON represents an object, calling code should add a key if there's other objects to stream
	/// </summary>
	/// <param name="stream">Stream to write to</param>
	/// <param name="mode">DTC_LinkEnableMode to parse</param>
	/// <returns>Stream object for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_LinkEnableMode& mode)
	{
		auto formatSet = (stream.flags() & std::ios_base::boolalpha) != 0;
		stream.setf(std::ios_base::boolalpha);
		stream << "{\"TransmitEnable\":" << mode.TransmitEnable << ",\"ReceiveEnable\":" << mode.ReceiveEnable
			   << ",\"TimingEnable\":" << mode.TimingEnable << "}";
		if (!formatSet) stream.unsetf(std::ios_base::boolalpha);
		return stream;
	}

	/// <summary>
	/// Determine if two DTC_LinkEnableMode objects are equal
	/// They are equal if TX, RX and Timing bits are all equal
	/// </summary>
	/// <param name="left">LHS of compare</param>
	/// <param name="right">RHS of compare</param>
	/// <returns>Whether all three bits of both sides are equal</returns>
	friend bool operator==(const DTC_LinkEnableMode& left, const DTC_LinkEnableMode& right)
	{
		return left.TransmitEnable == right.TransmitEnable && left.ReceiveEnable == right.ReceiveEnable &&
			   left.TimingEnable == right.TimingEnable;
	}

	/// <summary>
	/// Determine if two DTC_LinkEnableMode objects are not equal
	/// </summary>
	/// <param name="left">LHS of compare</param>
	/// <param name="right">RHS of compare</param>
	/// <returns>!(left == right)</returns>
	friend bool operator!=(const DTC_LinkEnableMode& left, const DTC_LinkEnableMode& right) { return !(left == right); }
};

enum DTC_IICSERDESBusAddress : uint8_t
{
	DTC_IICSERDESBusAddress_EVB = 0x55,
	DTC_IICSERDESBusAddress_CFO = 0x5d,
	DTC_IICSERDESBusAddress_JitterAttenuator = 0x68,
};

enum DTC_IICDDRBusAddress : uint8_t
{
	DTC_IICDDRBusAddress_DDROscillator = 0x59,
};

/// <summary>
/// This structure is used to decode the FIFOFullErrorFlags register values
/// </summary>
struct DTC_FIFOFullErrorFlags
{
	bool OutputData;            ///< Output Data FIFO Full
	bool CFOLinkInput;          ///< CFO Link Input FIFO Full
	bool ReadoutRequestOutput;  ///< Readout Request Output FIFO Full
	bool DataRequestOutput;     ///< Data Request Output FIFO Full
	bool OtherOutput;           ///< Other Output FIFO Full
	bool OutputDCS;             ///< Output DCS FIFO Full
	bool OutputDCSStage2;       ///< Output DCS Stage 2 FIFO Full
	bool DataInput;             ///< Data Input FIFO Full
	bool DCSStatusInput;        ///< DCS Status Input FIFO Full

	/// <summary>
	/// Default Constructor, sets all flags to false
	/// </summary>
	DTC_FIFOFullErrorFlags()
		: OutputData(false), CFOLinkInput(false), ReadoutRequestOutput(false), DataRequestOutput(false), OtherOutput(false), OutputDCS(false), OutputDCSStage2(false), DataInput(false), DCSStatusInput(false) {}

	/// <summary>
	/// Construct a DTC_FIFOFUllErrorFlags instance with the given values
	/// </summary>
	/// <param name="outputData">Output Data FIFO Full</param>
	/// <param name="cfoLinkInput">CFO Link Input FIFO Full</param>
	/// <param name="readoutRequest">Readout Request FIFO Full</param>
	/// <param name="dataRequest">Data Request FIFO Full</param>
	/// <param name="otherOutput"> Other Output FIFO Full</param>
	/// <param name="outputDCS">Output DCS FIFO Full</param>
	/// <param name="outputDCS2">Output DCS Stage 2 FIFO Full</param>
	/// <param name="dataInput">Data Input FIFO Full</param>
	/// <param name="dcsInput">DCS Status Input FIFO Full</param>
	DTC_FIFOFullErrorFlags(bool outputData, bool cfoLinkInput, bool readoutRequest, bool dataRequest, bool otherOutput,
						   bool outputDCS, bool outputDCS2, bool dataInput, bool dcsInput)
		: OutputData(outputData), CFOLinkInput(cfoLinkInput), ReadoutRequestOutput(readoutRequest), DataRequestOutput(dataRequest), OtherOutput(otherOutput), OutputDCS(outputDCS), OutputDCSStage2(outputDCS2), DataInput(dataInput), DCSStatusInput(dcsInput) {}

	/// <summary>
	/// Write the DTC_FIFOFullErrorFlags to stream in JSON format.
	/// Note that the JSON represents an object, calling code should add a key if there's other objects to stream
	/// </summary>
	/// <param name="stream">Stream to write to</param>
	/// <param name="flags">Flags to parse</param>
	/// <returns>Stream object for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_FIFOFullErrorFlags& flags)
	{
		auto formatSet = (stream.flags() & std::ios_base::boolalpha) != 0;
		stream.setf(std::ios_base::boolalpha);
		stream << "{\"OutputData\":" << flags.OutputData << ",\"CFOLinkInput\":" << flags.CFOLinkInput
			   << ",\"ReadoutRequestOutput\":" << flags.ReadoutRequestOutput
			   << ",\"DataRequestOutput\":" << flags.DataRequestOutput << ",\"OtherOutput\":" << flags.OtherOutput
			   << ",\"OutputDCS\":" << flags.OutputDCS << ",\"OutputDCSStage2\":" << flags.OutputDCSStage2
			   << ",\"DataInput\":" << flags.DataInput << ",\"DCSStatusInput\":" << flags.DCSStatusInput << "}";
		if (!formatSet) stream.unsetf(std::ios_base::boolalpha);
		return stream;
	}
};

/// <summary>
/// This structure is used to decode the DDR Flags register values
/// </summary>
struct DTC_DDRFlags
{
	bool InputFragmentBufferFull;       ///< The input fragment buffer is full
	bool InputFragmentBufferFullError;  ///< The input fragment buffer is full and in error state
	bool InputFragmentBufferEmpty;      ///< The input fragment buffer is empty
	bool InputFragmentBufferHalfFull;   ///< The input fragment buffer is at least half full
	bool OutputEventBufferFull;         ///< The output event buffer is full
	bool OutputEventBufferFullError;    ///< The output event buffer is full and in error state
	bool OutputEventBufferEmpty;        ///< The output event buffer is empty
	bool OutputEventBufferHalfFull;     ///< The output event buffer is at least half full

	/// <summary>
	/// Default Constructor, sets all flags to false
	/// </summary>
	DTC_DDRFlags()
		: InputFragmentBufferFull(false), InputFragmentBufferFullError(false), InputFragmentBufferEmpty(false), InputFragmentBufferHalfFull(false), OutputEventBufferFull(false), OutputEventBufferFullError(false), OutputEventBufferEmpty(false), OutputEventBufferHalfFull(false) {}

	/// <summary>
	/// Construct a DTC_DDRFlags instance with the given values
	/// </summary>
	/// <param name="ifbf">InputFragmentBufferFull value</param>
	/// <param name="ifbfe">InputFragmentBufferFullError</param>
	/// <param name="ifbe">InputFragmentBufferEmpty</param>
	/// <param name="ifbhf">InputFragmentBufferHalfFull</param>
	/// <param name="ofbf">OutputEventBufferFull</param>
	/// <param name="ofbfe">OutputEventBufferFullError</param>
	/// <param name="ofbe">OutputEventBufferEmpty</param>
	/// <param name="ofbhf">OutputEventBufferHalfFull</param>
	DTC_DDRFlags(bool ifbf, bool ifbfe, bool ifbe, bool ifbhf, bool ofbf, bool ofbfe, bool ofbe, bool ofbhf)
		: InputFragmentBufferFull(ifbf), InputFragmentBufferFullError(ifbfe), InputFragmentBufferEmpty(ifbe), InputFragmentBufferHalfFull(ifbhf), OutputEventBufferFull(ofbf), OutputEventBufferFullError(ofbfe), OutputEventBufferEmpty(ofbe), OutputEventBufferHalfFull(ofbhf) {}

	/// <summary>
	/// Write the DTC_DDRFlags to stream in JSON format.
	/// Note that the JSON represents an object, calling code should add a key if there's other objects to stream
	/// </summary>
	/// <param name="stream">Stream to write to</param>
	/// <param name="flags">Flags to parse</param>
	/// <returns>Stream object for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_DDRFlags& flags)
	{
		auto formatSet = (stream.flags() & std::ios_base::boolalpha) != 0;
		stream.setf(std::ios_base::boolalpha);
		stream << "{\"InputFragmentBufferFull\":" << flags.InputFragmentBufferFull
			   << ",\"InputFragmentBufferFullError\":" << flags.InputFragmentBufferFullError
			   << ",\"InputFragmentBufferEmpty\":" << flags.InputFragmentBufferEmpty
			   << ",\"InputFragmentBufferHalfFull\":" << flags.InputFragmentBufferHalfFull
			   << ",\"OutputEventBufferFull\":" << flags.OutputEventBufferFull
			   << ",\"OutputEventBufferFullError\":" << flags.OutputEventBufferFullError
			   << ",\"OutputEventBufferEmpty\":" << flags.OutputEventBufferEmpty
			   << ",\"OutputEventBufferHalfFull\":" << flags.OutputEventBufferHalfFull << "}";
		if (!formatSet) stream.unsetf(std::ios_base::boolalpha);
		return stream;
	}
};

/// <summary>
/// The DTC_RegisterFormatter class is used to print a DTC register in a human-readable format
/// </summary>
struct DTC_RegisterFormatter
{
	/// <summary>
	/// Default Constructor with zero values
	/// </summary>
	DTC_RegisterFormatter()
		: address(0), value(0), descWidth(28), description(""), vals() {}

	/// <summary>
	/// Default Copy Consturctor
	/// </summary>
	/// <param name="r">DTC_RegisterFormatter to copy</param>
	DTC_RegisterFormatter(const DTC_RegisterFormatter& r) = default;
	/// <summary>
	/// Default Move Constructor
	/// </summary>
	/// <param name="r">DTC_RegisterFormatter rvalue</param>
	DTC_RegisterFormatter(DTC_RegisterFormatter&& r) = default;
	uint16_t address;               ///< Address of the register
	uint32_t value;                 ///< Value of the register
	int descWidth;                  ///< Display width of description field
	std::string description;        ///< Description of the register (name)
	std::vector<std::string> vals;  ///< Human-readable descriptions of register values (bits, etc)

	/// <summary>
	/// Default Copy Assignment Operator
	/// </summary>
	/// <param name="r">RHS</param>
	/// <returns>DTC_RegisterFormatter regerence</returns>
	DTC_RegisterFormatter& operator=(const DTC_RegisterFormatter& r) = default;

	/// <summary>
	/// Write out the DTC_RegisterFormatter to the given stream. This function uses setw to make sure that fields for
	/// different registers still line up.
	/// Format is: "    0xADDR  | 0xVALUEXXX | DESCRIPTION: Variable size, minimum of 28 chars | Value 1
	///                                                                                        | Value 2 ..."
	/// </summary>
	/// <param name="stream">Stream to write register data to</param>
	/// <param name="reg">RegisterFormatter to write</param>
	/// <returns>Stream reference for continued streaming</returns>
	friend std::ostream& operator<<(std::ostream& stream, const DTC_RegisterFormatter& reg)
	{
		stream << std::hex << std::setfill('0');
		stream << "    0x" << std::setw(4) << static_cast<int>(reg.address) << "  | 0x" << std::setw(8)
			   << static_cast<int>(reg.value) << " | ";
		auto tmp = reg.description;
		tmp.resize(reg.descWidth, ' ');
		stream << tmp << " | ";

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

		return stream;
	}
};

/// <summary>
/// Several useful data manipulation utilities
/// </summary>
struct Utilities
{
	/// <summary>
	/// Create a string with "[value] [unit]" for a given number of bytes, using FormatBytes(bytes)
	/// </summary>
	/// <param name="bytes">Number of bytes to convert to string</param>
	/// <param name="extraUnit">Extra units to be applied after converted byte unit (i.e. "/s")</param>
	/// <returns>String with converted value</returns>
	static std::string FormatByteString(double bytes, std::string extraUnit);
	/// <summary>
	/// Determine the best units for describing a given number of bytes.
	/// Algorithm will divide by 1024, and if the result is greater than 1 and less than 1024, use that unit.
	/// Maximum unit is TB.
	/// </summary>
	/// <param name="bytes">Number of bytes to format</param>
	/// <returns>Pair of Value in "best unit" and string representation of unit (i.e. "KB", "MB", etc)</returns>
	static std::pair<double, std::string> FormatBytes(double bytes);
	/// <summary>
	/// Create a string with "[value] [unit]" for a given number of seconds, using FormatTime(seconds)
	/// </summary>
	/// <param name="seconds">Number of seconds to convert to string</param>
	/// <returns>String with converted value</returns>
	static std::string FormatTimeString(double seconds);
	/// <summary>
	/// Determine the best units for describing a given number of seconds.
	/// Will find largest time span which has unit greater than 1, up to days, down to ns.
	/// </summary>
	/// <param name="seconds">Number of seconds to convert</param>
	/// <returns>Pair of Value in "best unit" and string representation of unit (i.e. "ns", "us", "hours", etc)</returns>
	static std::pair<double, std::string> FormatTime(double seconds);

	/// <summary>
	/// Print out the buffer in hexdump -c format
	/// </summary>
	/// <param name="ptr">Pointer to the buffer</param>
	/// <param name="sz">Size of the buffer</param>
	/// <param name="quietCount">Number of lines to print at the begin/end. Default is 0, which prints entire buffer</param>
	/// <param name="tlvl">TLVL to use for printing (Default TLVL_INFO)</param>
	static void PrintBuffer(void* ptr, size_t sz, size_t quietCount = 0, int tlvl = TLVL_INFO);
};
}  // namespace DTCLib

#endif  // DTC_TYPES_H
