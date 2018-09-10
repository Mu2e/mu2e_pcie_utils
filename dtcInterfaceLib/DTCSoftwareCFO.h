///////////////////////////////////////////////////////////////////////////////
// The DTCSoftwareCFO class is responsible for sending ReadoutRequest and
// DataRequest packets to the DTC in the absence of a functioning CFO.
// Requests are sent asynchronously, and the overall behaviour is meant to
// emulate a system with a CFO as closely as possible.
//
// Author: Eric Flumerfelt, FNAL RSI
// Date: 8/13/2015
///////////////////////////////////////////////////////////////////////////////

#ifndef DTCSOFTWARECFO_H
#define DTCSOFTWARECFO_H 1

#include "DTC.h"
#include "DTC_Types.h"

#include <thread>
#include <atomic>
#include <set>

namespace DTCLib
{
	/// <summary>
	/// DEPRECATED
	/// The DTCSoftwareCFO class is responsible for sending ReadoutRequest and
	/// DataRequest packets to the DTC in the absence of a functioning CFO.
	/// Requests are sent asynchronously, and the overall behaviour is meant to
	/// emulate a system with a CFO as closely as possible.
	/// </summary>
	class DTCSoftwareCFO
	{
	public:
		/// <summary>
		/// Construct an instance of the DTCSoftwareCFO class
		/// </summary>
		/// <param name="dtc">Pointer to DTC object to use for sending requests</param>
		/// <param name="useCFOEmulator">Whether to use the DTC CFO emulator</param>
		/// <param name="debugPacketCount">Number of debug packets to request</param>
		/// <param name="debugType">Flag for the debug packets</param>
		/// <param name="stickyDebugType">Whether the debug type should stay as the flag or revert to default</param>
		/// <param name="quiet">When true, don't print debug information to screen</param>
		/// <param name="asyncRR">Whether to send ReadoutRequests asynchronously</param>
		/// <param name="forceNoDebugMode">Do NOT set the Debug flag in Data Request</param>
		DTCSoftwareCFO(DTC* dtc, bool useCFOEmulator, uint16_t debugPacketCount = 0,
		               DTC_DebugType debugType = DTC_DebugType_ExternalSerialWithReset, bool stickyDebugType = false,
		               bool quiet = false, bool asyncRR = false, bool forceNoDebugMode = false);
		/// <summary>
		/// DTCSoftwareCFO Destructor
		/// </summary>
		~DTCSoftwareCFO();

		/// <summary>
		/// Send a readout request and data request for a given timestamp
		/// </summary>
		/// <param name="ts">Timestamp for requests</param>
		void SendRequestForTimestamp(DTC_Timestamp ts = DTC_Timestamp(static_cast<uint64_t>(0)));
		/// <summary>
		/// Send requests for a range of timestamps.
		/// </summary>
		/// <param name="count">Number of requests</param>
		/// <param name="start">Starting timestamp (Default: 0)</param>
		/// <param name="increment">Whether to increment the timestamp for each request (Default: true)</param>
		/// <param name="delayBetweenDataRequests">Number of microseconds to wait between requests</param>
		/// <param name="requestsAhead">Number of readout requests to send ahead of data requests</param>
		void SendRequestsForRange(int count, DTC_Timestamp start = DTC_Timestamp(static_cast<uint64_t>(0)),
		                          bool increment = true, uint32_t delayBetweenDataRequests = 0, int requestsAhead = 1);

		/// <summary>
		/// Send requests for a list of timestamps.
		/// </summary>
		/// <param name="timestamps">List of timestamps to send</param>
		/// <param name="delayBetweenDataRequests">Number of microseconds to wait between requests</param>
		void SendRequestsForList(std::set<DTC_Timestamp> timestamps, uint32_t delayBetweenDataRequests = 0);

		/// <summary>
		/// Enable quiet mode.
		///
		/// When in quiet mode, DTCSoftwareCFO will not print ReadoutRequest packets or DataRequest packets to screen.
		/// </summary>
		/// <param name="quiet">Whether to enable quiet mode.</param>
		void setQuiet(bool quiet)
		{
			quiet_ = quiet;
		}

		/// <summary>
		/// Set the number of packets requested
		/// </summary>
		/// <param name="dbc">Packet count</param>
		void setDebugPacketCount(uint16_t dbc)
		{
			debugPacketCount_ = dbc;
		}

		/// <summary>
		/// Blocks until all ReadoutRequest packets have been sent
		/// </summary>
		void WaitForRequestsToBeSent() const;

		/// <summary>
		/// Get a pointer to the DTC instance
		/// </summary>
		/// <returns>Pointer to the DTC instance</returns>
		DTC* GetDTC() const
		{
			return theDTC_;
		}

	private:
		void SendRequestsForRangeImplAsync(DTC_Timestamp start, int count,
		                                   bool increment = true, uint32_t delayBetweenDataRequests = 0);
		void SendRequestsForRangeImplSync(DTC_Timestamp start, int count,
		                                  bool increment = true, uint32_t delayBetweenDataRequests = 0, int requestsAhead = 1);

		void SendRequestsForListImplAsync(std::set<DTC_Timestamp> timestamps, uint32_t delayBetweenDataRequests = 0);

		// Request Parameters
		bool useCFOEmulator_;
		uint16_t debugPacketCount_;
		DTC_DebugType debugType_;
		bool stickyDebugType_;
		bool quiet_; // Don't print as much
		bool asyncRR_;
		bool forceNoDebug_;

		// Object basic properties (not accessible)
		DTC* theDTC_;
		DTC_LinkEnableMode linkMode_[6];
		std::unique_ptr<std::thread> theThread_;
		std::atomic<bool> requestsSent_;
		std::atomic<bool> abort_;
	};
} // namespace DTCLib
#endif //ifndef DTCSOFTWARECFO_H


