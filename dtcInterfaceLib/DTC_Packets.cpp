#include "DTC_Packets.h"

#include <cstring>
#include <iomanip>
#include <sstream>

DTCLib::DTC_DataPacket::DTC_DataPacket()
{
	memPacket_ = false;
	vals_ = std::vector<uint8_t>(16);
	dataPtr_ = &vals_[0];
	dataSize_ = 16;
}

DTCLib::DTC_DataPacket::DTC_DataPacket(const DTC_DataPacket& in)
{
	dataSize_ = in.GetSize();
	memPacket_ = in.IsMemoryPacket();
	if (!memPacket_)
	{
		vals_ = std::vector<uint8_t>(dataSize_);
		dataPtr_ = &vals_[0];
		memcpy(const_cast<uint8_t*>(dataPtr_), in.GetData(), in.GetSize() * sizeof(uint8_t));
	}
	else
	{
		dataPtr_ = in.GetData();
	}
}

DTCLib::DTC_DataPacket::~DTC_DataPacket()
{
	if (!memPacket_ && dataPtr_ != nullptr)
	{
		dataPtr_ = nullptr;
	}
}

void DTCLib::DTC_DataPacket::SetWord(uint16_t index, uint8_t data)
{
	if (!memPacket_ && index < dataSize_)
	{
		const_cast<uint8_t*>(dataPtr_)[index] = data;
	}
}

uint8_t DTCLib::DTC_DataPacket::GetWord(uint16_t index) const
{
	if (index < dataSize_) return dataPtr_[index];
	return 0;
}

bool DTCLib::DTC_DataPacket::Resize(const uint16_t dmaSize)
{
	if (!memPacket_ && dmaSize > dataSize_)
	{
		vals_.resize(dmaSize);
		dataPtr_ = &vals_[0];
		dataSize_ = dmaSize;
		return true;
	}

	// We can only grow, and only non-memory-mapped packets
	return false;
}

std::string DTCLib::DTC_DataPacket::toJSON() const
{
	std::stringstream ss;
	ss << "\"DataPacket\": {";
	ss << "\"data\": [";
	ss << std::hex << std::setfill('0');
	uint16_t jj = 0;
	for (uint16_t ii = 0; ii < dataSize_ - 2; ii += 2)
	{
		ss << "0x" << std::setw(4) << static_cast<int>(reinterpret_cast<uint16_t const*>(dataPtr_)[jj]) << ",";
		++jj;
	}
	ss << "0x" << std::setw(4) << static_cast<int>(reinterpret_cast<uint16_t const*>(dataPtr_)[dataSize_ - 2]) << "]";
	ss << "}";
	return ss.str();
}

std::string DTCLib::DTC_DataPacket::toPacketFormat() const
{
	std::stringstream ss;
	ss << std::setfill('0') << std::hex;
	for (uint16_t ii = 0; ii < dataSize_ - 1; ii += 2)
	{
		ss << "0x" << std::setw(2) << static_cast<int>(dataPtr_[ii + 1]) << " ";
		ss << "" << std::setw(2) << static_cast<int>(dataPtr_[ii]) << "\n";
	}
	ss << std::dec;
	return ss.str();
}

bool DTCLib::DTC_DataPacket::Equals(const DTC_DataPacket& other) const
{
	auto equal = true;
	for (uint16_t ii = 2; ii < 16; ++ii)
	{
		// TRACE(21, "DTC_DataPacket::Equals: Compalink %u to %u", GetWord(ii), other.GetWord(ii));
		if (other.GetWord(ii) != GetWord(ii))
		{
			equal = false;
			break;
		}
	}

	return equal;
}

DTCLib::DTC_DMAPacket::DTC_DMAPacket(DTC_PacketType type, DTC_Link_ID link, uint16_t byteCount, bool valid, uint8_t subsystemID, uint8_t hopCount)
	: byteCount_(byteCount), valid_(valid), subsystemID_(subsystemID), linkID_(link), packetType_(type), hopCount_(hopCount) {}

DTCLib::DTC_DataPacket DTCLib::DTC_DMAPacket::ConvertToDataPacket() const
{
	DTC_DataPacket output;
	output.Resize(byteCount_);
	auto word0A = static_cast<uint8_t>(byteCount_);
	auto word0B = static_cast<uint8_t>(byteCount_ >> 8);
	output.SetWord(0, word0A);
	output.SetWord(1, word0B);
	auto word1A = static_cast<uint8_t>(hopCount_ & 0xF);
	word1A += static_cast<uint8_t>(packetType_) << 4;
	uint8_t word1B = static_cast<uint8_t>(linkID_) + (valid_ ? 0x80 : 0x0);
	output.SetWord(2, word1A);
	output.SetWord(3, word1B);
	for (uint16_t i = 4; i < byteCount_; ++i)
	{
		output.SetWord(i, 0);
	}

	//std::cout << "ConvertToDataPacket: \n"
	//		  << output.toPacketFormat() << std::endl;

	return output;
}

DTCLib::DTC_DMAPacket::DTC_DMAPacket(const DTC_DataPacket in)
{
	auto word2 = in.GetData()[2];
	uint8_t hopCount = word2 & 0xF;
	uint8_t packetType = word2 >> 4;
	auto word3 = in.GetData()[3];
	uint8_t linkID = word3 & 0xF;
	valid_ = (word3 & 0x80) == 0x80;
	subsystemID_ = (word3 >> 4) & 0x7;

	byteCount_ = in.GetData()[0] + (in.GetData()[1] << 8);
	hopCount_ = hopCount;
	linkID_ = static_cast<DTC_Link_ID>(linkID);
	packetType_ = static_cast<DTC_PacketType>(packetType);
	TLOG_ARB(20, "DTC_DMAPacket") << headerJSON();
}

std::string DTCLib::DTC_DMAPacket::headerJSON() const
{
	std::stringstream ss;
	ss << "\"byteCount\": " << std::hex << "0x" << static_cast<int>(byteCount_) << ",";
	ss << "\"isValid\": " << valid_ << ",";
	ss << "\"subsystemID\": " << std::hex << "0x" << static_cast<int>(subsystemID_) << ",";
	ss << "\"linkID\": " << std::dec << linkID_ << ",";
	ss << "\"packetType\": " << packetType_ << ",";

	ss << "\"hopCount\": " << std::hex << "0x" << static_cast<int>(hopCount_);

	return ss.str();
}

std::string DTCLib::DTC_DMAPacket::headerPacketFormat() const
{
	std::stringstream ss;
	ss << std::setfill('0') << std::hex;
	ss << "0x" << std::setw(6) << ((byteCount_ & 0xFF00) >> 8) << "\t"
	   << "0x" << std::setw(6) << (byteCount_ & 0xFF) << std::endl;
	ss << std::setw(1) << static_cast<int>(valid_) << " "
	   << std::setw(2) << std::dec << static_cast<int>(subsystemID_) << std::hex << " "
	   << "0x" << std::setw(2) << linkID_ << "\t";
	ss << "0x" << std::setw(2) << packetType_ << "0x" << std::setw(2) << 0 << std::endl;
	return ss.str();
}

std::string DTCLib::DTC_DMAPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"DMAPacket\": {";
	ss << headerJSON();
	ss << "}";
	return ss.str();
}

std::string DTCLib::DTC_DMAPacket::toPacketFormat() { return headerPacketFormat(); }

DTCLib::DTC_DCSRequestPacket::DTC_DCSRequestPacket()
	: DTC_DMAPacket(DTC_PacketType_DCSRequest, DTC_Link_Unused), type_(DTC_DCSOperationType_Unknown), packetCount_(0), address1_(0), data1_(0), address2_(0), data2_(0) {}

DTCLib::DTC_DCSRequestPacket::DTC_DCSRequestPacket(DTC_Link_ID link)
	: DTC_DMAPacket(DTC_PacketType_DCSRequest, link), type_(DTC_DCSOperationType_Unknown), packetCount_(0), address1_(0), data1_(0), address2_(0), data2_(0) {}

DTCLib::DTC_DCSRequestPacket::DTC_DCSRequestPacket(DTC_Link_ID link, DTC_DCSOperationType type, bool requestAck, bool incrementAddress,
												   uint16_t address, uint16_t data, uint16_t address2, uint16_t data2)
	: DTC_DMAPacket(DTC_PacketType_DCSRequest, link), type_(type), requestAck_(requestAck), incrementAddress_(incrementAddress), packetCount_(0), address1_(address), data1_(data), address2_(address2), data2_(data2) { UpdatePacketAndWordCounts(); }

DTCLib::DTC_DCSRequestPacket::DTC_DCSRequestPacket(DTC_DataPacket in)
	: DTC_DMAPacket(in)
{
	if (packetType_ != DTC_PacketType_DCSRequest)
	{
		auto ex = DTC_WrongPacketTypeException(DTC_PacketType_DCSRequest, packetType_);
		TLOG(TLVL_ERROR) << ex.what();
		throw ex;
	}
	type_ = static_cast<DTC_DCSOperationType>(in.GetData()[4] & 0x7);
	requestAck_ = (in.GetData()[4] & 0x8) == 0x8;
	incrementAddress_ = (in.GetData()[4] & 0x10) == 0x10;

	packetCount_ = (in.GetData()[4] >> 6) + (in.GetData()[5] << 2);
	address1_ = in.GetData()[6] + (in.GetData()[7] << 8);
	data1_ = in.GetData()[8] + (in.GetData()[9] << 8);

	if (type_ == DTC_DCSOperationType_BlockWrite)
	{
		address2_ = 0;
		data2_ = 0;
		blockWriteData_.push_back(in.GetData()[10] + (in.GetData()[11] << 8));
		blockWriteData_.push_back(in.GetData()[12] + (in.GetData()[13] << 8));
		blockWriteData_.push_back(in.GetData()[14] + (in.GetData()[15] << 8));
	}
	else
	{
		address2_ = in.GetData()[10] + (in.GetData()[11] << 8);
		data2_ = in.GetData()[12] + (in.GetData()[13] << 8);
	}

	std::cout << "Constructor copy: " << toJSON() << std::endl;
}

std::string DTCLib::DTC_DCSRequestPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"DCSRequestPacket\": {";
	ss << headerJSON() << ", ";
	ss << "\"Operation Type\":" << DTC_DCSOperationTypeConverter(type_) << ", ";
	ss << "\"Request Acknowledgement\":" << (requestAck_ ? "\"true\"" : "\"false\"") << ", ";
	ss << "\"Address1\": " << static_cast<int>(address1_) << ", ";
	if (type_ != DTC_DCSOperationType_BlockWrite)
	{
		ss << "\"Data1\": " << static_cast<int>(data1_) << ", ";
		ss << "\"Address2\": " << static_cast<int>(address2_) << ", ";
		ss << "\"Data2\": " << static_cast<int>(data2_);
	}
	else
	{
		auto counter = 0;
		ss << ", \"Block Word Count\": " << static_cast<int>(data1_);
		for (auto& word : blockWriteData_)
		{
			ss << ", "
			   << "\"Block Write word " << counter << "\":" << static_cast<int>(word);
			counter++;
		}
	}
	ss << "}";

	return ss.str();
}

std::string DTCLib::DTC_DCSRequestPacket::toPacketFormat()
{
	std::stringstream ss;
	ss << headerPacketFormat() << std::hex << std::setfill('0');

	auto firstWord = (packetCount_ & 0x3FC) >> 2;
	auto secondWord =
		((packetCount_ & 0x3) << 6) + (incrementAddress_ ? 0x10 : 0) + (requestAck_ ? 0x8 : 0) + (static_cast<int>(type_) & 0x7);
	ss << std::setw(8) << firstWord << "\t" << secondWord << std::endl;

	ss << std::setw(8) << ((address1_ & 0xFF00) >> 8) << "\t" << (address1_ & 0xFF) << std::endl;
	ss << std::setw(8) << ((data1_ & 0xFF00) >> 8) << "\t" << (data1_ & 0xFF) << std::endl;
	if (type_ != DTC_DCSOperationType_BlockWrite)
	{
		ss << std::setw(8) << ((address2_ & 0xFF00) >> 8) << "\t" << (address2_ & 0xFF) << std::endl;
		ss << std::setw(8) << ((data2_ & 0xFF00) >> 8) << "\t" << (data2_ & 0xFF) << std::endl;
		ss << "        \t        " << std::endl;
	}
	else
	{
		if (blockWriteData_.size() > 0)
		{
			ss << std::setw(8) << ((blockWriteData_[0] & 0xFF00) >> 8) << "\t" << (blockWriteData_[0] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
		if (blockWriteData_.size() > 1)
		{
			ss << std::setw(8) << ((blockWriteData_[1] & 0xFF00) >> 8) << "\t" << (blockWriteData_[1] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
		if (blockWriteData_.size() > 2)
		{
			ss << std::setw(8) << ((blockWriteData_[2] & 0xFF00) >> 8) << "\t" << (blockWriteData_[2] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
	}
	return ss.str();
}

void DTCLib::DTC_DCSRequestPacket::AddRequest(uint16_t address, uint16_t data)
{
	if (IsDoubleOp())
	{
		auto ex = DTC_IOErrorException(255);
		TLOG(TLVL_ERROR) << "DCS Request already has two requests, cannot add another! " << ex.what();
		throw ex;
	}
	if (type_ == DTC_DCSOperationType_BlockRead || type_ == DTC_DCSOperationType_BlockWrite)
	{
		auto ex = DTC_IOErrorException(254);
		TLOG(TLVL_ERROR) << "Cannot add second request to Block Read or Block Write operations! " << ex.what();
		throw ex;
	}
	type_ = static_cast<DTC_DCSOperationType>(type_ | 0x4);
	address2_ = address;
	data2_ = data;
}

void DTCLib::DTC_DCSRequestPacket::UpdatePacketAndWordCounts()
{
	assert(blockWriteData_.size() < 0x10000);

	if (type_ == DTC_DCSOperationType_BlockWrite)
	{
		data1_ = blockWriteData_.size();
	}

	if (type_ == DTC_DCSOperationType_BlockWrite)
	{
		packetCount_ = (data1_ - 3) / 8 + ((data1_ - 3) % 8 ? 1 : 0);
	}
	else
	{
		packetCount_ = 0;
	}
}

DTCLib::DTC_DataPacket DTCLib::DTC_DCSRequestPacket::ConvertToDataPacket() const
{
	auto output = DTC_DMAPacket::ConvertToDataPacket();

	auto type = type_;
	if (address2_ == 0 && data2_ == 0 && (type_ == DTC_DCSOperationType_DoubleRead || type_ == DTC_DCSOperationType_DoubleWrite))
	{
		type = static_cast<DTC_DCSOperationType>(type_ & 0x1);
	}

	auto firstWord = (packetCount_ & 0x3FC) >> 2;
	auto secondWord =
		((packetCount_ & 0x3) << 6) + (incrementAddress_ ? 0x10 : 0) + (requestAck_ ? 0x8 : 0) + (static_cast<int>(type) & 0x7);
	output.SetWord(4, static_cast<uint8_t>(secondWord));
	output.SetWord(5, static_cast<uint8_t>(firstWord));

	output.SetWord(6, static_cast<uint8_t>(address1_ & 0xFF));
	output.SetWord(7, static_cast<uint8_t>(((address1_ & 0xFF00) >> 8)));
	output.SetWord(8, static_cast<uint8_t>(data1_ & 0xFF));
	output.SetWord(9, static_cast<uint8_t>(((data1_ & 0xFF00) >> 8)));

	if (type != DTC_DCSOperationType_BlockWrite)
	{
		output.SetWord(10, static_cast<uint8_t>(address2_ & 0xFF));
		output.SetWord(11, static_cast<uint8_t>(((address2_ & 0xFF00) >> 8)));
		output.SetWord(12, static_cast<uint8_t>(data2_ & 0xFF));
		output.SetWord(13, static_cast<uint8_t>(((data2_ & 0xFF00) >> 8)));
		output.SetWord(14, 0);
		output.SetWord(15, 0);
	}
	else
	{
		output.Resize((1 + packetCount_) * 16);
		size_t wordCounter = 10;
		for (auto& word : blockWriteData_)
		{
			output.SetWord(wordCounter, word & 0xFF);
			output.SetWord(wordCounter + 1, (word & 0xFF00) >> 8);
			wordCounter += 2;
		}
		for (; wordCounter < static_cast<size_t>((1 + packetCount_) * 16); wordCounter++)
		{
			output.SetWord(wordCounter, 0);
		}
	}
	return output;
}

DTCLib::DTC_HeartbeatPacket::DTC_HeartbeatPacket(DTC_Link_ID link)
	: DTC_DMAPacket(DTC_PacketType_Heartbeat, link), timestamp_(), eventMode_()
{
	eventMode_[0] = 0;
	eventMode_[1] = 0;
	eventMode_[2] = 0;
	eventMode_[3] = 0;
	eventMode_[4] = 0;
	eventMode_[5] = 0;
}

DTCLib::DTC_HeartbeatPacket::DTC_HeartbeatPacket(DTC_Link_ID link, DTC_Timestamp timestamp, uint8_t* eventMode)
	: DTC_DMAPacket(DTC_PacketType_Heartbeat, link), timestamp_(timestamp), eventMode_()
{
	if (eventMode != nullptr)
	{
		for (auto i = 0; i < 6; ++i)
		{
			eventMode_[i] = eventMode[i];
		}
	}
}

DTCLib::DTC_HeartbeatPacket::DTC_HeartbeatPacket(const DTC_DataPacket in)
	: DTC_DMAPacket(in)
{
	if (packetType_ != DTC_PacketType_Heartbeat)
	{
		auto ex = DTC_WrongPacketTypeException(DTC_PacketType_Heartbeat, packetType_);
		TLOG(TLVL_ERROR) << ex.what();
		throw ex;
	}
	auto arr = in.GetData();
	eventMode_[0] = arr[10];
	eventMode_[1] = arr[11];
	eventMode_[2] = arr[12];
	eventMode_[3] = arr[13];
	eventMode_[4] = arr[14];
	eventMode_[5] = arr[15];
	timestamp_ = DTC_Timestamp(arr, 4);
}

std::string DTCLib::DTC_HeartbeatPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"ReadoutRequestPacket\": {";
	ss << headerJSON() << ",";
	ss << timestamp_.toJSON() << ",";
	ss << "\"request\": [" << std::hex << "0x" << static_cast<int>(eventMode_[0]) << ",";
	ss << std::hex << "0x" << static_cast<int>(eventMode_[1]) << ",";
	ss << std::hex << "0x" << static_cast<int>(eventMode_[2]) << ",";
	ss << std::hex << "0x" << static_cast<int>(eventMode_[3]) << ",";
	ss << std::hex << "0x" << static_cast<int>(eventMode_[4]) << ",";
	ss << std::hex << "0x" << static_cast<int>(eventMode_[5]) << "],";
	ss << "}";
	return ss.str();
}

std::string DTCLib::DTC_HeartbeatPacket::toPacketFormat()
{
	std::stringstream ss;
	ss << headerPacketFormat() << std::setfill('0') << std::hex;
	ss << timestamp_.toPacketFormat();
	ss << "0x" << std::setw(6) << static_cast<int>(eventMode_[1]) << "\t0x" << std::setw(6)
	   << static_cast<int>(eventMode_[0]) << "\n";
	ss << "0x" << std::setw(6) << static_cast<int>(eventMode_[3]) << "\t0x" << std::setw(6)
	   << static_cast<int>(eventMode_[2]) << "\n";
	ss << "0x" << std::setw(6) << static_cast<int>(eventMode_[5]) << "\t0x" << std::setw(6)
	   << static_cast<int>(eventMode_[4]) << "\n";
	return ss.str();
}

DTCLib::DTC_DataPacket DTCLib::DTC_HeartbeatPacket::ConvertToDataPacket() const
{
	auto output = DTC_DMAPacket::ConvertToDataPacket();
	timestamp_.GetTimestamp(output.GetData(), 4);
	for (auto i = 0; i < 6; ++i)
	{
		output.SetWord(static_cast<uint16_t>(10 + i), eventMode_[i]);
	}
	return output;
}

DTCLib::DTC_DataRequestPacket::DTC_DataRequestPacket(DTC_Link_ID link, bool debug, uint16_t debugPacketCount,
													 DTC_DebugType type)
	: DTC_DMAPacket(DTC_PacketType_DataRequest, link), timestamp_(), debug_(debug), debugPacketCount_(debugPacketCount), type_(type) {}

DTCLib::DTC_DataRequestPacket::DTC_DataRequestPacket(DTC_Link_ID link, DTC_Timestamp timestamp, bool debug,
													 uint16_t debugPacketCount, DTC_DebugType type)
	: DTC_DMAPacket(DTC_PacketType_DataRequest, link), timestamp_(timestamp), debug_(debug), debugPacketCount_(debugPacketCount), type_(type) {}

DTCLib::DTC_DataRequestPacket::DTC_DataRequestPacket(DTC_DataPacket in)
	: DTC_DMAPacket(in)
{
	if (packetType_ != DTC_PacketType_DataRequest)
	{
		auto ex = DTC_WrongPacketTypeException(DTC_PacketType_DataRequest, packetType_);
		TLOG(TLVL_ERROR) << ex.what();
		throw ex;
	}
	timestamp_ = DTC_Timestamp(in.GetData(), 4);
	debug_ = (in.GetData()[12] & 0x1) == 1;
	type_ = DTC_DebugType((in.GetData()[12] & 0xF0) >> 4);
	debugPacketCount_ = in.GetData()[14] + (in.GetData()[15] << 8);
}

std::string DTCLib::DTC_DataRequestPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"DataRequestPacket\": {";
	ss << headerJSON() << ",";
	ss << timestamp_.toJSON() << ",";
	ss << "\"debug\":" << (debug_ ? "true" : "false") << ",";
	ss << "\"debugPacketCount\": " << std::dec << static_cast<int>(debugPacketCount_) << ",";
	ss << DTC_DebugTypeConverter(type_);
	ss << "}";
	return ss.str();
}

std::string DTCLib::DTC_DataRequestPacket::toPacketFormat()
{
	std::stringstream ss;
	ss << headerPacketFormat() << std::setfill('0') << std::hex;
	ss << timestamp_.toPacketFormat();
	ss << "        \t        \n";
	ss << "        \t0x" << std::setw(2) << static_cast<int>(type_) << "   " << std::setw(1) << static_cast<int>(debug_)
	   << "\n";
	ss << "0x" << std::setw(6) << ((debugPacketCount_ & 0xFF00) >> 8) << "\t"
	   << "0x" << std::setw(6) << (debugPacketCount_ & 0xFF) << "\n";
	return ss.str();
}

DTCLib::DTC_DataPacket DTCLib::DTC_DataRequestPacket::ConvertToDataPacket() const
{
	auto output = DTC_DMAPacket::ConvertToDataPacket();
	timestamp_.GetTimestamp(output.GetData(), 4);
	output.SetWord(12, (static_cast<uint8_t>(type_) << 4) + (debug_ ? 1 : 0));
	output.SetWord(14, debugPacketCount_ & 0xFF);
	output.SetWord(15, (debugPacketCount_ >> 8) & 0xFF);
	return output;
}

void DTCLib::DTC_DataRequestPacket::SetDebugPacketCount(uint16_t count)
{
	if (count > 0)
	{
		debug_ = true;
	}
	else
	{
		debug_ = false;
	}
	debugPacketCount_ = count;
}

DTCLib::DTC_DCSReplyPacket::DTC_DCSReplyPacket(DTC_DataPacket in)
	: DTC_DMAPacket(in)
{
	TRACE(20, "DTC_DCSReplyPacket::DTC_DCSReplyPacket Before packetType test");
	if (packetType_ != DTC_PacketType_DCSReply)
	{
		auto ex = DTC_WrongPacketTypeException(DTC_PacketType_DCSReply, packetType_);
		TLOG(TLVL_ERROR) << ex.what();
		throw ex;
	}
	type_ = static_cast<DTC_DCSOperationType>(in.GetData()[4] & 0x3);
	doubleOp_ = (in.GetData()[4] & 0x4) == 0x4;
	requestAck_ = (in.GetData()[4] & 0x8) == 0x8;

	dcsReceiveFIFOEmpty_ = (in.GetData()[4] & 0x10) == 0x10;
	corruptFlag_ = (in.GetData()[4] & 0x20) == 0x20;

	packetCount_ = (in.GetData()[4] >> 6) + (in.GetData()[5] << 2);
	address1_ = in.GetData()[6] + (in.GetData()[7] << 8);
	data1_ = in.GetData()[8] + (in.GetData()[9] << 8);

	if (type_ == DTC_DCSOperationType_BlockRead)
	{
		address2_ = 0;
		data2_ = 0;
		if (data1_ > 0)
		{
			blockReadData_.push_back(in.GetData()[10] + (in.GetData()[11] << 8));
		}
		if (data1_ > 1)
		{
			blockReadData_.push_back(in.GetData()[12] + (in.GetData()[13] << 8));
		}
		if (data1_ > 2)
		{
			blockReadData_.push_back(in.GetData()[14] + (in.GetData()[15] << 8));
		}

		if (in.GetSize() > 16)
		{
			size_t wordCounter = 16;
			while (wordCounter < in.GetSize())
			{
				blockReadData_.push_back(in.GetData()[wordCounter] + (in.GetData()[wordCounter + 1] << 8));
				wordCounter += 2;
			}
		}
	}
	else
	{
		address2_ = in.GetData()[10] + (in.GetData()[11] << 8);
		data2_ = in.GetData()[12] + (in.GetData()[13] << 8);
	}
}

std::string DTCLib::DTC_DCSReplyPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"DCSReplyPacket\": {";
	ss << headerJSON() << ", ";
	ss << "\"Operation Type\":" << DTC_DCSOperationTypeConverter(type_) << ", ";
	ss << "\"Double Operation\":" << (doubleOp_ ? "\"true\"" : "\"false\"") << ", ";
	ss << "\"Request Acknowledgement\":" << (requestAck_ ? "\"true\"" : "\"false\"") << ", ";
	ss << "\"DCS Request FIFO Empty\": " << (dcsReceiveFIFOEmpty_ ? "\"true\"" : "\"false\"") << ", ";
	ss << "\"Corrupt Flag\": " << (corruptFlag_ ? "\"true\"" : "\"false\"") << ", ";
	ss << "\"Address1\": " << static_cast<int>(address1_) << ", ";
	if (type_ != DTC_DCSOperationType_BlockRead)
	{
		ss << "\"Data1\": " << static_cast<int>(data1_) << ", ";
		ss << "\"Address2\": " << static_cast<int>(address2_) << ", ";
		ss << "\"Data2\": " << static_cast<int>(data2_);
	}
	else
	{
		ss << "\"Block Word Count\": " << static_cast<int>(data1_);
		auto counter = 0;
		for (auto& word : blockReadData_)
		{
			ss << ", "
			   << "\"Block Read word " << counter << "\":" << static_cast<int>(word);
			counter++;
		}
	}
	ss << "}";
	return ss.str();
}

std::string DTCLib::DTC_DCSReplyPacket::toPacketFormat()
{
	std::stringstream ss;
	ss << headerPacketFormat() << std::hex << std::setfill('0');

	auto firstWord = (packetCount_ & 0x3FC) >> 2;
	auto secondWord = ((packetCount_ & 0x3) << 6) + (corruptFlag_ ? 0x20 : 0) + (dcsReceiveFIFOEmpty_ ? 0x10 : 0) +
					  (requestAck_ ? 0x8 : 0) + (doubleOp_ ? 0x4 : 0) + static_cast<int>(type_);
	ss << std::setw(8) << firstWord << "\t" << secondWord << std::endl;

	ss << std::setw(8) << ((address1_ & 0xFF00) >> 8) << "\t" << (address1_ & 0xFF) << std::endl;
	ss << std::setw(8) << ((data1_ & 0xFF00) >> 8) << "\t" << (data1_ & 0xFF) << std::endl;
	if (type_ != DTC_DCSOperationType_BlockRead)
	{
		ss << std::setw(8) << ((address2_ & 0xFF00) >> 8) << "\t" << (address2_ & 0xFF) << std::endl;
		ss << std::setw(8) << ((data2_ & 0xFF00) >> 8) << "\t" << (data2_ & 0xFF) << std::endl;
		ss << "        \t        " << std::endl;
	}
	else
	{
		if (blockReadData_.size() > 0)
		{
			ss << std::setw(8) << ((blockReadData_[0] & 0xFF00) >> 8) << "\t" << (blockReadData_[0] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
		if (blockReadData_.size() > 1)
		{
			ss << std::setw(8) << ((blockReadData_[1] & 0xFF00) >> 8) << "\t" << (blockReadData_[1] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
		if (blockReadData_.size() > 2)
		{
			ss << std::setw(8) << ((blockReadData_[2] & 0xFF00) >> 8) << "\t" << (blockReadData_[2] & 0xFF) << std::endl;
		}
		else
		{
			ss << "        \t        " << std::endl;
		}
	}
	return ss.str();
}

DTCLib::DTC_DataPacket DTCLib::DTC_DCSReplyPacket::ConvertToDataPacket() const
{
	auto output = DTC_DMAPacket::ConvertToDataPacket();

	auto firstWord = (packetCount_ & 0x3FC) >> 2;
	auto secondWord = ((packetCount_ & 0x3) << 6) + (corruptFlag_ ? 0x20 : 0) + (dcsReceiveFIFOEmpty_ ? 0x10 : 0) +
					  (requestAck_ ? 0x8 : 0) + (doubleOp_ ? 0x4 : 0) + static_cast<int>(type_);
	output.SetWord(4, static_cast<uint8_t>(secondWord));
	output.SetWord(5, static_cast<uint8_t>(firstWord));

	output.SetWord(6, static_cast<uint8_t>(address1_ & 0xFF));
	output.SetWord(7, static_cast<uint8_t>(((address1_ & 0xFF00) >> 8)));
	output.SetWord(8, static_cast<uint8_t>(data1_ & 0xFF));
	output.SetWord(9, static_cast<uint8_t>(((data1_ & 0xFF00) >> 8)));

	if (type_ != DTC_DCSOperationType_BlockRead)
	{
		output.SetWord(10, static_cast<uint8_t>(address2_ & 0xFF));
		output.SetWord(11, static_cast<uint8_t>(((address2_ & 0xFF00) >> 8)));
		output.SetWord(12, static_cast<uint8_t>(data2_ & 0xFF));
		output.SetWord(13, static_cast<uint8_t>(((data2_ & 0xFF00) >> 8)));
	}
	else
	{
		output.Resize((1 + packetCount_) * 16);
		size_t wordCounter = 10;
		for (auto& word : blockReadData_)
		{
			output.SetWord(wordCounter, word & 0xFF);
			output.SetWord(wordCounter + 1, (word & 0xFF00) >> 8);
			wordCounter += 2;
		}
	}
	return output;
}

DTCLib::DTC_DataHeaderPacket::DTC_DataHeaderPacket(DTC_Link_ID link, uint16_t packetCount, DTC_DataStatus status,
												   uint8_t dtcid, DTC_Subsystem subsystemid, uint8_t packetVersion, DTC_Timestamp timestamp,
												   uint8_t evbMode)
	: DTC_DMAPacket(DTC_PacketType_DataHeader, link, (1 + packetCount) * 16, true, subsystemid), packetCount_(packetCount), timestamp_(timestamp), status_(status), dataPacketVersion_(packetVersion), dtcId_(dtcid), evbMode_(evbMode) {}

DTCLib::DTC_DataHeaderPacket::DTC_DataHeaderPacket(DTC_DataPacket in)
	: DTC_DMAPacket(in)
{
	if (packetType_ != DTC_PacketType_DataHeader)
	{
		auto ex = DTC_WrongPacketTypeException(DTC_PacketType_DataHeader, packetType_);
		TLOG(TLVL_ERROR) << "Unexpected packet type encountered: " + std::to_string(packetType_) + " != " + std::to_string(DTC_PacketType_DataHeader) +
								" (expected)";
		TLOG(TLVL_DEBUG) << "Packet contents: " << in.toJSON();
		throw ex;
	}
	auto arr = in.GetData();
	packetCount_ = arr[4] + (arr[5] << 8);
	timestamp_ = DTC_Timestamp(arr, 6);
	status_ = DTC_DataStatus(arr[12]);
	dataPacketVersion_ = arr[13];
	dtcId_ = arr[14];
	evbMode_ = arr[15];
}

std::string DTCLib::DTC_DataHeaderPacket::toJSON()
{
	std::stringstream ss;
	ss << "\"DataHeaderPacket\": {";
	ss << headerJSON() << ",";
	ss << "\"packetCount\": " << std::dec << static_cast<int>(packetCount_) << ",";
	ss << timestamp_.toJSON() << ",";
	ss << "\"status\": " << std::dec << static_cast<int>(status_) << ",";
	ss << "\"packetVersion\": " << std::hex << static_cast<int>(dataPacketVersion_) << ",";
	ss << "\"DTC ID\": " << std::dec << static_cast<int>(dtcId_) << ",";
	ss << "\"evbMode\": " << std::hex << "0x" << static_cast<int>(evbMode_) << "}";
	return ss.str();
}

std::string DTCLib::DTC_DataHeaderPacket::toPacketFormat()
{
	std::stringstream ss;
	ss << headerPacketFormat() << std::setfill('0') << std::hex;
	ss << "     0x" << std::setw(1) << ((packetCount_ & 0x0700) >> 8) << "\t"
	   << "0x" << std::setw(6) << (packetCount_ & 0xFF) << "\n";
	ss << timestamp_.toPacketFormat();
	ss << "0x" << std::setw(6) << static_cast<int>(dataPacketVersion_) << "\t"
	   << "0x" << std::setw(6) << static_cast<int>(status_) << "\n";
	ss << "0x" << std::setw(6) << static_cast<int>(evbMode_) << "\t" << std::dec << std::setw(8) << static_cast<int>(dtcId_) << "\n";
	return ss.str();
}

DTCLib::DTC_DataPacket DTCLib::DTC_DataHeaderPacket::ConvertToDataPacket() const
{
	auto output = DTC_DMAPacket::ConvertToDataPacket();
	output.SetWord(4, static_cast<uint8_t>(packetCount_));
	output.SetWord(5, static_cast<uint8_t>((packetCount_ & 0x0700) >> 8));
	timestamp_.GetTimestamp(output.GetData(), 6);
	output.SetWord(12, static_cast<uint8_t>(status_));
	output.SetWord(13, static_cast<uint8_t>(dataPacketVersion_));
	output.SetWord(14, static_cast<uint8_t>(dtcId_));
	output.SetWord(15, evbMode_);
	return output;
}

bool DTCLib::DTC_DataHeaderPacket::Equals(const DTC_DataHeaderPacket& other) const
{
	return ConvertToDataPacket() == other.ConvertToDataPacket();
}
