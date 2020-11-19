#include "DTC_Types.h"

#include <iomanip>
#include <sstream>
#include <cmath>

#include "TRACE/tracemf.h"

DTCLib::DTC_RXStatusConverter::DTC_RXStatusConverter(DTC_RXStatus status)
	: status_(status) {}

DTCLib::DTC_DebugType DTCLib::DTC_DebugTypeConverter::ConvertToDebugType(std::string const& type)
{
	switch (type[0])
	{
		case '0':
		case 's':
		case 'S':
			return DTC_DebugType_SpecialSequence;
		case '1':
		case 'e':
		case 'E':
			return DTC_DebugType_ExternalSerial;
		case '2':
		case 'w':
		case 'W':
			return DTC_DebugType_ExternalSerialWithReset;
		case '3':
		case 'r':
		case 'R':
			return DTC_DebugType_RAMTest;
		case '4':
		case 'd':
		case 'D':
			return DTC_DebugType_DDRTest;
		default:
			return DTC_DebugType_Invalid;
	}
}

DTCLib::DTC_SimMode DTCLib::DTC_SimModeConverter::ConvertToSimMode(std::string modeName)
{
	switch (modeName[0])
	{
		case '1':
		case 't':
		case 'T':
			return DTC_SimMode_Tracker;
		case '2':
		case 'c':
		case 'C':
			return DTC_SimMode_Calorimeter;
		case '3':
		case 'v':
		case 'V':
			return DTC_SimMode_CosmicVeto;
		case '4':
		case 'n':
		case 'N':
			return DTC_SimMode_NoCFO;
		case '5':
		case 'r':
		case 'R':
			return DTC_SimMode_ROCEmulator;
		case '6':
		case 'l':
		case 'L':
			return DTC_SimMode_Loopback;
		case '7':
		case 'p':
		case 'P':
			return DTC_SimMode_Performance;
		case '8':
		case 'f':
		case 'F':
			return DTC_SimMode_LargeFile;
		case '9':
		case 'o':
		case 'O':
			return DTC_SimMode_Timeout;
		case '0':
			return DTC_SimMode_Disabled;
		default:
			break;
	}

	try
	{
		auto modeInt = static_cast<DTC_SimMode>(stoi(modeName, nullptr, 10));
		return modeInt != DTC_SimMode_Invalid ? modeInt : DTC_SimMode_Disabled;
	}
	catch (...)
	{
		return DTC_SimMode_Invalid;
	}
}

DTCLib::DTC_Timestamp::DTC_Timestamp()
	: timestamp_(0) {}

DTCLib::DTC_Timestamp::DTC_Timestamp(const uint64_t timestamp)
	: timestamp_(timestamp) {}

DTCLib::DTC_Timestamp::DTC_Timestamp(const uint32_t timestampLow, const uint16_t timestampHigh)
{
	SetTimestamp(timestampLow, timestampHigh);
}

DTCLib::DTC_Timestamp::DTC_Timestamp(const uint8_t* timeArr, int offset)
{
	auto arr = reinterpret_cast<const uint64_t*>(timeArr + offset);
	timestamp_ = *arr;
}

DTCLib::DTC_Timestamp::DTC_Timestamp(const std::bitset<48> timestamp)
	: timestamp_(timestamp.to_ullong()) {}

void DTCLib::DTC_Timestamp::SetTimestamp(const uint32_t timestampLow, const uint16_t timestampHigh)
{
	uint64_t timestamp_temp = timestampHigh;
	timestamp_temp = timestamp_temp << 32;
	timestamp_ = timestampLow + timestamp_temp;
}

void DTCLib::DTC_Timestamp::GetTimestamp(const uint8_t* timeArr, int offset) const
{
	for (auto i = 0; i < 6; i++)
	{
		const_cast<uint8_t*>(timeArr)[i + offset] = static_cast<uint8_t>(timestamp_ >> i * 8);
	}
}

std::string DTCLib::DTC_Timestamp::toJSON(bool arrayMode) const
{
	std::stringstream ss;
	if (arrayMode)
	{
		uint8_t ts[6];
		GetTimestamp(ts, 0);
		ss << "\"timestamp\": [" << static_cast<int>(ts[0]) << ",";
		ss << static_cast<int>(ts[1]) << ",";
		ss << static_cast<int>(ts[2]) << ",";
		ss << static_cast<int>(ts[3]) << ",";
		ss << static_cast<int>(ts[4]) << ",";
		ss << static_cast<int>(ts[5]) << "]";
	}
	else
	{
		ss << "\"timestamp\": " << timestamp_;
	}
	return ss.str();
}

std::string DTCLib::DTC_Timestamp::toPacketFormat() const
{
	uint8_t ts[6];
	GetTimestamp(ts, 0);
	std::stringstream ss;
	ss << std::setfill('0') << std::hex;
	ss << "0x" << std::setw(6) << static_cast<int>(ts[1]) << "\t"
	   << "0x" << std::setw(6) << static_cast<int>(ts[0]) << "\n";
	ss << "0x" << std::setw(6) << static_cast<int>(ts[3]) << "\t"
	   << "0x" << std::setw(6) << static_cast<int>(ts[2]) << "\n";
	ss << "0x" << std::setw(6) << static_cast<int>(ts[5]) << "\t"
	   << "0x" << std::setw(6) << static_cast<int>(ts[4]) << "\n";
	return ss.str();
}

DTCLib::DTC_SERDESRXDisparityError::DTC_SERDESRXDisparityError()
	: data_(0) {}

DTCLib::DTC_SERDESRXDisparityError::DTC_SERDESRXDisparityError(std::bitset<2> data)
	: data_(data) {}

DTCLib::DTC_SERDESRXDisparityError::DTC_SERDESRXDisparityError(uint32_t data, DTC_Link_ID link)
{
	std::bitset<32> dataSet = data;
	uint32_t linkBase = static_cast<uint8_t>(link) * 2;
	data_[0] = dataSet[linkBase];
	data_[1] = dataSet[linkBase + 1];
}

DTCLib::DTC_CharacterNotInTableError::DTC_CharacterNotInTableError()
	: data_(0) {}

DTCLib::DTC_CharacterNotInTableError::DTC_CharacterNotInTableError(std::bitset<2> data)
	: data_(data) {}

DTCLib::DTC_CharacterNotInTableError::DTC_CharacterNotInTableError(uint32_t data, DTC_Link_ID link)
{
	std::bitset<32> dataSet = data;
	uint32_t linkBase = static_cast<uint8_t>(link) * 2;
	data_[0] = dataSet[linkBase];
	data_[1] = dataSet[linkBase + 1];
}

std::string DTCLib::Utilities::FormatByteString(double bytes, std::string extraUnit)
{
	auto res = FormatBytes(bytes);
	std::stringstream s;
	s << std::setprecision(5) << res.first << " " << res.second << extraUnit << " ("
	  << std::to_string(static_cast<uint64_t>(bytes)) << " bytes" << extraUnit << ")";
	return s.str();
}

std::pair<double, std::string> DTCLib::Utilities::FormatBytes(double bytes)
{
	auto val = bytes;
	auto unit = "bytes";
	auto kb = bytes / 1024.0;

	if (kb > 1)
	{
		auto mb = kb / 1024.0;
		if (mb > 1)
		{
			auto gb = mb / 1024.0;
			if (gb > 1)
			{
				auto tb = gb / 1024.0;
				if (tb > 1)
				{
					val = tb;
					unit = "TB";
				}
				else
				{
					val = gb;
					unit = "GB";
				}
			}
			else
			{
				val = mb;
				unit = "MB";
			}
		}
		else
		{
			val = kb;
			unit = "KB";
		}
	}

	return std::make_pair(val, unit);
}

std::string DTCLib::Utilities::FormatTimeString(double seconds)
{
	auto res = FormatTime(seconds);
	std::stringstream s;
	s << std::setprecision(5) << res.first << " " << res.second;
	return s.str();
}

std::pair<double, std::string> DTCLib::Utilities::FormatTime(double seconds)
{
	auto val = seconds;
	auto unit = "s";

	if (seconds > 1)
	{
		auto min = seconds / 60.0;
		if (min > 1)
		{
			auto ho = min / 60.0;
			if (ho > 1)
			{
				auto day = ho / 24.0;
				if (day > 1)
				{
					val = day;
					unit = "days";
				}
				else
				{
					val = ho;
					unit = "hours";
				}
			}
			else
			{
				val = min;
				unit = "minutes";
			}
		}
	}
	else
	{
		auto ms = seconds * 1000;
		if (ms > 1)
		{
			val = ms;
			unit = "ms";
		}
		else
		{
			auto us = ms * 1000;
			if (us > 1)
			{
				val = us;
				unit = "us";
			}
			else
			{
				auto ns = us * 1000;
				val = ns;
				unit = "ns";
			}
		}
	}

	return std::make_pair(val, unit);
}

void DTCLib::Utilities::PrintBuffer(void* ptr, size_t sz, size_t quietCount, int tlvl)
{
	auto maxLine = static_cast<unsigned>(ceil((sz - 8) / 16.0));
	for (unsigned line = 0; line < maxLine; ++line)
	{
		std::stringstream ostr;
		ostr << "0x" << std::hex << std::setw(5) << std::setfill('0') << line << "0: ";
		for (unsigned byte = 0; byte < 8; ++byte)
		{
			if (line * 16 + 2 * byte < sz - 8u)
			{
				auto thisWord = reinterpret_cast<uint16_t*>(ptr)[4 + line * 8 + byte];
				ostr << std::setw(4) << static_cast<int>(thisWord) << " ";
			}
		}
		TLOG(tlvl) << ostr.str();
		if (quietCount > 0 && maxLine > quietCount * 2 && line == (quietCount - 1))
		{
			line = static_cast<unsigned>(ceil((sz - 8) / 16.0)) - (1 + quietCount);
		}
	}
}
