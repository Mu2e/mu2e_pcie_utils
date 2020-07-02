// mu2esim.hh: mu2e DTC simulator main file
//
// Eric Flumerfelt
// January 27, 2015
//
#ifndef MU2ESIM_HH
#define MU2ESIM_HH 1

#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "DTC_Packets.h"
#include "DTC_Types.h"
#include "mu2e_driver/mu2e_mmap_ioctl.h"  //

#define SIM_BUFFCOUNT 40U

/// <summary>
/// The mu2esim class emulates a DTC in software. It can be used for hardware-independent testing of software,
/// especially higher-level trigger algorithms.
/// </summary>
class mu2esim
{
public:
	/// <summary>
	/// Construct the mu2esim class. Initializes register space and zeroes out memory.
	/// </summary>
	mu2esim(std::string ddrFileName);
	~mu2esim();
	/// <summary>
	/// Initialize the simulator using the given simulation mode
	/// </summary>
	/// <param name="mode">Simulation mode to set</param>
	/// <returns>0 when successful (always)</returns>
	int init(DTCLib::DTC_SimMode mode = DTCLib::DTC_SimMode_Tracker);
	/// <summary>
	/// Reads data from the simulated DDR memory or using a packet emulator based on the simulation mode selected.
	/// Returns the number of bytes read. Negative values indicate errors.
	/// </summary>
	/// <param name="chn">Channel to read</param>
	/// <param name="buffer">Pointer to output buffer</param>
	/// <param name="tmo_ms">Timeout for read</param>
	/// <returns>Byte count of data read into buffer. Negative value indicates error.</returns>
	int read_data(int chn, void** buffer, int tmo_ms);
	/// <summary>
	/// Write data from the given buffer to the requested channel. The simulator will process the packets and enqueue
	/// appropriate responses.
	/// </summary>
	/// <param name="chn">Channel to write data to</param>
	/// <param name="buffer">Address of buffer to write</param>
	/// <param name="bytes">Bytes to write</param>
	/// <returns>0 when successful (always)</returns>
	int write_data(int chn, void* buffer, size_t bytes);
	/// <summary>
	/// Release a number of buffers held by the software on the given channel
	/// </summary>
	/// <param name="chn">Channel to release</param>
	/// <param name="num">Number of buffers to release</param>
	/// <returns>0 when successful (always)</returns>
	int read_release(int chn, unsigned num);
	/// <summary>
	/// Release all buffers held by the software on the given channel
	/// </summary>
	/// <param name="chn">Channel to release</param>
	/// <returns>0 when successful (always)</returns>
	int release_all(int chn);
	/// <summary>
	/// Read from the simulated register space
	/// </summary>
	/// <param name="address">Address to read</param>
	/// <param name="tmo_ms">timeout for read</param>
	/// <param name="output">Output pointer</param>
	/// <returns>0 when successful (always)</returns>
	int read_register(uint16_t address, int tmo_ms, uint32_t* output);
	/// <summary>
	/// Write to the simulated register space
	/// </summary>
	/// <param name="address">Address to write</param>
	/// <param name="tmo_ms">Timeout for write</param>
	/// <param name="data">Data to write</param>
	/// <returns>0 when successful (always)</returns>
	int write_register(uint16_t address, int tmo_ms, uint32_t data);

private:
	unsigned delta_(int chn, int dir);
	static void clearBuffer_(int chn, bool increment = true);
	void openEvent_(DTCLib::DTC_Timestamp ts);
	void closeEvent_();
	void CFOEmulator_();
	void packetSimulator_(DTCLib::DTC_Timestamp ts, DTCLib::DTC_Link_ID link, uint16_t packetCount);
	void dcsPacketSimulator_(DTCLib::DTC_DCSRequestPacket in);

	std::unordered_map<uint16_t, uint32_t> registers_;
	unsigned swIdx_[MU2E_MAX_CHANNELS];
	unsigned hwIdx_[MU2E_MAX_CHANNELS];
	//uint32_t detSimLoopCount_;
	mu2e_databuff_t* dmaData_[MU2E_MAX_CHANNELS][SIM_BUFFCOUNT];
	std::string ddrFileName_;
	std::fstream ddrFile_;
	DTCLib::DTC_SimMode mode_;
	uint16_t simIndex_[6];
	std::thread cfoEmulatorThread_;
	bool cancelCFO_;

	typedef std::bitset<6> readoutRequestData;
	std::map<uint64_t, readoutRequestData> readoutRequestReceived_;

	DTCLib::DTC_Timestamp currentTimestamp_;
	uint64_t currentEventSize_;
	std::streampos eventBegin_;
};

#endif
