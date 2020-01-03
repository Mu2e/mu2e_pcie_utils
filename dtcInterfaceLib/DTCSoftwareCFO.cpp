
#include "TRACE/tracemf.h"

//#define TRACE_NAME "DTCSoftwareCFO"
//#define TRACE_NAME (strstr(&__FILE__[0], "/srcs/") ? strstr(&__FILE__[0], "/srcs/") + 6 : __FILE__)  /* TOO LONG */
#define TRACE_NAME &std::string(__FILE__).substr(std::string(__FILE__).rfind('/', std::string(__FILE__).rfind('/') - 1) + 1)[0]
#include <iostream>
#include "DTCSoftwareCFO.h"

#define Q(X) #X
#define QUOTE(X) Q(X)
#define VAL(X) QUOTE(X) << " = " << X

DTCLib::DTCSoftwareCFO::DTCSoftwareCFO(DTC* dtc, bool useCFOEmulator, uint16_t debugPacketCount,
									   DTC_DebugType debugType, bool stickyDebugType, bool quiet, bool asyncRR,
									   bool forceNoDebug)
	: useCFOEmulator_(useCFOEmulator), debugPacketCount_(debugPacketCount), debugType_(debugType), stickyDebugType_(stickyDebugType), quiet_(quiet), asyncRR_(asyncRR), forceNoDebug_(forceNoDebug), theThread_(nullptr), requestsSent_(false), abort_(false)
{
	theDTC_ = dtc;
	for (auto link : DTC_Links)
	{
		linkMode_[link] = theDTC_->ReadLinkEnabled(link);
	}
	theDTC_->EnableAutogenDRP();
	theDTC_->SetAllEventModeWords(1U);
	theDTC_->SetEventModeWord(0, 0U);
}

DTCLib::DTCSoftwareCFO::~DTCSoftwareCFO()
{
	theDTC_->DisableAutogenDRP();
	abort_ = true;
	if (theThread_ && theThread_->joinable()) theThread_->join();
}

void DTCLib::DTCSoftwareCFO::WaitForRequestsToBeSent() const
{
	while (!requestsSent_)
	{
		usleep(1000);
	}
}

void DTCLib::DTCSoftwareCFO::SendRequestForTimestamp(DTC_Timestamp ts)
{
	if (theDTC_->IsDetectorEmulatorInUse())
	{
		TLOG(10) << "Enabling Detector Emulator for 1 DMA";
		// theDTC_->ResetDTC();
		theDTC_->DisableDetectorEmulator();
		theDTC_->SetDetectorEmulationDMACount(1);
		theDTC_->EnableDetectorEmulator();
		return;
	}
	if (!useCFOEmulator_)
	{
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable && linkMode_[link].TransmitEnable)
			{
				TLOG(11) << "SendRequestForTimestamp before SendReadoutRequestPacket";
				theDTC_->SendReadoutRequestPacket(link, ts, quiet_);
				int maxRoc;
				if ((maxRoc = theDTC_->ReadLinkROCCount(link)) != 0)
				{
					for (uint8_t roc = 0; roc <= maxRoc; ++roc)
					{
						TLOG(11) << "SendRequestForTimestamp before DTC_DataRequestPacket req";
						DTC_DataRequestPacket req(link, ts, !forceNoDebug_, debugPacketCount_, debugType_);
						if (debugType_ == DTC_DebugType_ExternalSerialWithReset && !stickyDebugType_)
						{
							debugType_ = DTC_DebugType_ExternalSerial;
						}
						TLOG(11) << "SendRequestForTimestamp before WriteDMADAQPacket - DTC_DataRequestPacket";
						if (!quiet_) std::cout << req.toJSON() << std::endl;
						theDTC_->WriteDMAPacket(req);
						TLOG(11) << "SendRequestForTimestamp after  WriteDMADAQPacket - DTC_DataRequestPacket";
					}
				}
				// usleep(2000);
			}
		}
	}
	else
	{
		TLOG(12) << "SendRequestForTimestamp setting up DTC CFO Emulator";
		theDTC_->SetCFOEmulationMode();
		theDTC_->DisableCFOEmulation();
		theDTC_->SetCFOEmulationTimestamp(ts);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable && linkMode_[link].TransmitEnable)
			{
				theDTC_->SetCFOEmulationNumPackets(link, debugPacketCount_);
			}
		}
		theDTC_->SetCFOEmulationNumRequests(1);
		//theDTC_->SetCFOEmulationRequestInterval(1000);
		theDTC_->SetCFOEmulationDebugType(debugType_);
		theDTC_->SetCFOEmulationModeByte(5, 1);
		if (!forceNoDebug_)
			theDTC_->EnableDebugPacketMode();
		else
			theDTC_->DisableDebugPacketMode();
		TLOG(12) << "SendRequestForTimestamp enabling DTC CFO Emulator";
		theDTC_->EnableCFOEmulation();
		TLOG(12) << "SendRequestForTimestamp done";
	}
	requestsSent_ = true;
}

void DTCLib::DTCSoftwareCFO::SendRequestsForRange(int count, DTC_Timestamp start, bool increment,
												  uint32_t delayBetweenDataRequests, int requestsAhead,
												  uint32_t readoutRequestsAfter)
{
	if (theDTC_->IsDetectorEmulatorInUse())
	{
		TLOG(13) << "Enabling Detector Emulator for " << count << " DMAs";

		// theDTC_->ResetDTC();
		theDTC_->DisableDetectorEmulator();
		theDTC_->SetDetectorEmulationDMACount(count + 1);
		theDTC_->EnableDetectorEmulator();
		return;
	}

	if (delayBetweenDataRequests < 1000)
	{
		delayBetweenDataRequests = 1000;
	}

	TLOG(2) << VAL(count);
	TLOG(2) << VAL(delayBetweenDataRequests);
	TLOG(2) << VAL(requestsAhead);
	TLOG(2) << VAL(readoutRequestsAfter);

	if (!useCFOEmulator_)
	{
		requestsSent_ = false;
		if (asyncRR_)
		{
			if (theThread_ && theThread_->joinable()) theThread_->join();
			theThread_.reset(new std::thread(&DTCSoftwareCFO::SendRequestsForRangeImplAsync, this, start, count, increment,
											 delayBetweenDataRequests, readoutRequestsAfter));
		}
		else
		{
			if (theThread_ && theThread_->joinable()) theThread_->join();
			theThread_.reset(new std::thread(&DTCSoftwareCFO::SendRequestsForRangeImplSync, this, start, count, increment,
											 delayBetweenDataRequests, requestsAhead, readoutRequestsAfter));
		}
		WaitForRequestsToBeSent();
	}
	else
	{
		TLOG(13) << "SendRequestsForRange setting up DTC CFO Emulator";

		theDTC_->SetCFOEmulationMode();
		theDTC_->DisableCFOEmulation();
		theDTC_->SetCFOEmulationTimestamp(start);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable && linkMode_[link].TransmitEnable)
			{
				theDTC_->SetCFOEmulationNumPackets(link, debugPacketCount_);
			}
		}
		theDTC_->SetCFOEmulationNumRequests(count);
		theDTC_->SetCFOEmulationDebugType(debugType_);
		theDTC_->SetCFOEmulationModeByte(5, 1);
		theDTC_->SetCFOEmulationNumNullHeartbeats(readoutRequestsAfter);
		if (!forceNoDebug_)
			theDTC_->EnableDebugPacketMode();
		else
			theDTC_->DisableDebugPacketMode();
		theDTC_->SetCFOEmulationRequestInterval(delayBetweenDataRequests);
		TLOG(13) << "SendRequestsForRange enabling DTC CFO Emulator";
		theDTC_->EnableCFOEmulation();
		TLOG(13) << "SendRequestsForRange done";
	}
}

void DTCLib::DTCSoftwareCFO::SendRequestsForList(std::set<DTC_Timestamp> timestamps,
												 uint32_t delayBetweenDataRequests)
{
	if (theDTC_->IsDetectorEmulatorInUse())
	{
		TLOG(16) << "Enabling Detector Emulator for " << timestamps.size() << " DMAs";
		// theDTC_->ResetDTC();
		theDTC_->DisableDetectorEmulator();
		theDTC_->SetDetectorEmulationDMACount(timestamps.size() + 1);
		theDTC_->EnableDetectorEmulator();
		return;
	}

	if (theThread_ && theThread_->joinable()) theThread_->join();
	theThread_.reset(
		new std::thread(&DTCSoftwareCFO::SendRequestsForListImplAsync, this, timestamps, delayBetweenDataRequests));
}

void DTCLib::DTCSoftwareCFO::SendRequestsForListImplAsync(std::set<DTC_Timestamp> timestamps,
														  uint32_t delayBetweenDataRequests)
{
	if (delayBetweenDataRequests < 1000)
	{
		delayBetweenDataRequests = 1000;
	}

	auto ii = timestamps.begin();
	while (ii != timestamps.end() && !abort_)
	{
		TLOG(16) << "Setting up CFO Emulator for next entry in list";
		auto thisTimestamp = *ii;
		++ii;
		DTC_Timestamp nextTimestamp = thisTimestamp + 5;  // Generate 5 nulls at the end of the list
		if (ii != timestamps.end())
		{
			nextTimestamp = *ii;
		}

		theDTC_->SetCFOEmulationMode();
		theDTC_->DisableCFOEmulation();
		theDTC_->SetCFOEmulationTimestamp(thisTimestamp);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable && linkMode_[link].TransmitEnable)
			{
				theDTC_->SetCFOEmulationNumPackets(link, debugPacketCount_);
			}
		}
		theDTC_->SetCFOEmulationNumRequests(1);
		theDTC_->SetCFOEmulationNumNullHeartbeats(nextTimestamp.GetTimestamp(true) - thisTimestamp.GetTimestamp(true) - 1);
		theDTC_->SetCFOEmulationDebugType(debugType_);
		theDTC_->SetCFOEmulationModeByte(5, 1);
		if (!forceNoDebug_)
			theDTC_->EnableDebugPacketMode();
		else
			theDTC_->DisableDebugPacketMode();
		theDTC_->SetCFOEmulationRequestInterval(delayBetweenDataRequests);
		TLOG(13) << "SendRequestsForRange enabling DTC CFO Emulator";
		theDTC_->EnableCFOEmulation();
		TLOG(13) << "SendRequestsForRange done";
	}
}

void DTCLib::DTCSoftwareCFO::SendRequestsForRangeImplSync(DTC_Timestamp start, int count, bool increment,
														  uint32_t delayBetweenDataRequests, int requestsAhead, uint32_t readoutRequestsAfter)
{
	TLOG(14) << "SendRequestsForRangeImplSync Start";
	for (auto ii = 0; ii < count; ++ii)
	{
		auto ts = start + (increment ? ii : 0);

		SendRequestForTimestamp(ts);
		if (ii >= requestsAhead || ii == count - 1)
		{
			requestsSent_ = true;
		}

		usleep(delayBetweenDataRequests);
		if (abort_) return;
	}

	for (uint32_t ii = 0; ii < readoutRequestsAfter; ++ii)
	{
		auto ts = start + (increment ? count + ii : 0);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable)
			{
				if (linkMode_[link].TransmitEnable)
				{
					TLOG(15) << "SendRequestsForRangeImpl before SendReadoutRequestPacket";
					theDTC_->SendReadoutRequestPacket(link, ts, quiet_);
				}
			}
			if (abort_) return;
		}
	}
}

void DTCLib::DTCSoftwareCFO::SendRequestsForRangeImplAsync(DTC_Timestamp start, int count, bool increment,
														   uint32_t delayBetweenDataRequests, uint32_t readoutRequestsAfter)
{
	TLOG(15) << "SendRequestsForRangeImplAsync Start";

	// Send Readout Requests first
	for (auto ii = 0; ii < count; ++ii)
	{
		auto ts = start + (increment ? ii : 0);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable)
			{
				if (linkMode_[link].TransmitEnable)
				{
					TLOG(15) << "SendRequestsForRangeImpl before SendReadoutRequestPacket";
					theDTC_->SendReadoutRequestPacket(link, ts, quiet_);
				}
			}
			if (abort_) return;
		}
	}
	TLOG(15) << "SendRequestsForRangeImpl setting RequestsSent flag";
	requestsSent_ = true;

	// Now do the DataRequests, sleeping for the required delay between each.
	for (auto ii = 0; ii < count; ++ii)
	{
		auto ts = start + (increment ? ii : 0);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable)
			{
				if (linkMode_[link].TransmitEnable)
				{
					int maxRoc;
					if ((maxRoc = theDTC_->ReadLinkROCCount(link)) != 0)
					{
						for (uint8_t roc = 0; roc <= maxRoc; ++roc)
						{
							TLOG(15) << "SendRequestsForRangeImpl before DTC_DataRequestPacket req";
							DTC_DataRequestPacket req(link, ts, !forceNoDebug_, static_cast<uint16_t>(debugPacketCount_), debugType_);
							if (debugType_ == DTC_DebugType_ExternalSerialWithReset && !stickyDebugType_)
							{
								debugType_ = DTC_DebugType_ExternalSerial;
							}
							TLOG(15) << "SendRequestsForRangeImpl before WriteDMADAQPacket - DTC_DataRequestPacket";
							if (!quiet_) std::cout << req.toJSON() << std::endl;
							theDTC_->WriteDMAPacket(req);
							TLOG(15) << "SendRequestsForRangeImpl after  WriteDMADAQPacket - DTC_DataRequestPacket";
							if (abort_) return;
						}
					}
					// usleep(2000);
				}
			}
			if (abort_) return;
		}
		usleep(delayBetweenDataRequests);
	}

	for (uint32_t ii = 0; ii < readoutRequestsAfter; ++ii)
	{
		auto ts = start + (increment ? count + ii : 0);
		for (auto link : DTC_Links)
		{
			if (!linkMode_[link].TimingEnable)
			{
				if (linkMode_[link].TransmitEnable)
				{
					TLOG(15) << "SendRequestsForRangeImpl before SendReadoutRequestPacket";
					theDTC_->SendReadoutRequestPacket(link, ts, quiet_);
				}
			}
			if (abort_) return;
		}
	}
}
