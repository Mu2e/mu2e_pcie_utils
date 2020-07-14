#ifndef DTC_H
#define DTC_H

#include <list>
#include <memory>
#include <vector>

#include "DTC_Packets.h"
#include "DTC_Registers.h"
#include "DTC_Types.h"

namespace DTCLib {
/// <summary>
/// The DTC class implements the data transfers to the DTC card. It derives from DTC_Registers, the class representing
/// the DTC register space.
/// </summary>
class DTC : public DTC_Registers
{
public:
	/// <summary>
	/// Construct an instance of the DTC class
	/// </summary>
	/// <param name="mode">The desired simulation mode for the DTC (Default: Disabled)</param>
	/// <param name="dtc">The DTC card to use (default: -1: Use environment variable or 0 if env. var. unset)</param>
	/// <param name="rocMask">Which ROCs should be active. Each hex digit corresponds to a Link, and the number indicates
	/// how many ROCs are active on that link. (Default: 0x1)</param> <param name="expectedDesignVersion">Expected DTC
	/// Firmware Design Version. If set, will throw an exception if the DTC firmware does not match (Default: "")</param>
	/// <param name="skipInit">Whether to skip full initialization of the DTC</param>
	explicit DTC(DTC_SimMode mode = DTC_SimMode_Disabled, int dtc = -1, unsigned rocMask = 0x1,
				 std::string expectedDesignVersion = "", bool skipInit = false, std::string simMemoryFile = "mu2esim.bin");
	virtual ~DTC();

	//
	// DMA Functions
	//
	// Data read-out
	/// <summary>
	/// Reads data from the DTC, and returns all data blocks with the same timestamp. If timestamp is specified, will look
	/// for data with that timestamp.
	/// </summary>
	/// <param name="when">Desired timestamp for readout. Default means use whatever timestamp is next</param>
	/// <returns>A vector of DTC_DataBlock objects</returns>
	std::vector<DTC_DataBlock> GetData(DTC_Timestamp when = DTC_Timestamp());
	/// <summary>
	/// Reads data from the DTC and returns all data blocks with the same timestamp, as a JSON string.
	/// </summary>
	/// <param name="when">Desired timestamp for readout. Default means use whatever timestamp is next</param>
	/// <returns>JSON data string</returns>
	std::string GetJSONData(DTC_Timestamp when = DTC_Timestamp());
	/// <summary>
	/// Read a file into the DTC memory. Will truncate the file so that it fits in the DTC memory.
	/// </summary>
	/// <param name="file">File name to read into DTC memory</param>
	/// <param name="goForever">Whether readout should loop through the file</param>
	/// <param name="overwriteEnvrionment">Whether to use file instead of DTCLIB_SIM_FILE</param>
	/// <param name="outputFileName">Name of binary file to write expected output (Default: "", no file created)</param>
	/// <param name="skipVerify">Skip the verify stage of WriteSimFileToDTC</param>
	void WriteSimFileToDTC(std::string file, bool goForever, bool overwriteEnvrionment = false,
						   std::string outputFileName = "", bool skipVerify = false);
	/// <summary>
	/// Read the DTC memory and determine whether the file was written correctly.
	/// </summary>
	/// <param name="file">File to verify against</param>
	/// <param name="rawOutputFilename">Default: "". If set, file to write data retrieved from the DTC to</param>
	/// <returns>True if file is in DTC memory without errors</returns>
	bool VerifySimFileInDTC(std::string file, std::string rawOutputFilename = "");

	// DCS Register R/W
	/// <summary>
	/// Sends a DCS Request Packet with fields filled in such that the given ROC register will be read out.
	/// This function reads from the main ROC register space, use ReadExtROCRegister to access other firmware blocks'
	/// register spaces.
	/// </summary>
	/// <param name="link">Link of the ROC to read</param>
	/// <param name="address">Address of the register</param>
	/// <param name="retries">Numberof times to retry when packet address or link does not match request</param>
	/// <returns>Value of the ROC register from a DCS Reply packet</returns>
	roc_data_t ReadROCRegister(const DTC_Link_ID& link, const roc_address_t address, int retries = 10);
	/// <summary>
	/// Sends a DCS Request Packet with the fields filled in such that the given ROC register will be written.
	/// This function writes to the main ROC register space, use WriteExtROCRegister to access other firmware blocks'
	/// register spaces.
	/// </summary>
	/// <param name="link">Link of the ROC to write to</param>
	/// <param name="address">Address of the register</param>
	/// <param name="data">Value to write</param>
	/// <param name="requestAck">Whether to request acknowledement of this operation</param>
	void WriteROCRegister(const DTC_Link_ID& link, const roc_address_t address, const roc_data_t data, bool requestAck);

	/// <summary>
	/// Perform a "double operation" read of ROC registers
	/// </summary>
	/// <param name="link">Link of the ROC to read</param>
	/// <param name="address1">First address to read</param>
	/// <param name="address2">Second address to read</param>
	/// <param name="retries">Numberof times to retry when packet address or link does not match request</param>
	/// <returns>Pair of register values, first is from the first address, second from the second</returns>
	std::pair<roc_data_t, roc_data_t> ReadROCRegisters(const DTC_Link_ID& link, const roc_address_t address1,
													   const roc_address_t address2, int retries = 10);
	/// <summary>
	/// Perform a "double operation" write to ROC registers
	/// </summary>
	/// <param name="link">Link of the ROC to write to</param>
	/// <param name="address1">First address to write</param>
	/// <param name="data1">Value to write to first register</param>
	/// <param name="address2">Second address to write</param>
	/// <param name="data2">Value to write to second register</param>
	/// <param name="requestAck">Whether to request acknowledement of this operation</param>
	void WriteROCRegisters(const DTC_Link_ID& link, const roc_address_t address1, const roc_data_t data1,
						   const roc_address_t address2, const roc_data_t data2, bool requestAck);
	/// <summary>
	/// Perform a ROC block read
	/// </summary>
	/// <param name="data">vector of data by reference to read</param>
	/// <param name="link">Link of the ROC to read</param>
	/// <param name="address">Address of the block</param>
	/// <param name="wordCount">Number of words to read</param>
	/// <returns>Vector of words returned by block read</returns>
	/// <param name="incrementAddress">Whether to increment the address pointer for block reads/writes</param>
	void ReadROCBlock(std::vector<roc_data_t>& data, const DTC_Link_ID& link, const roc_address_t address, const uint16_t wordCount, bool incrementAddress);
	/// <summary>
	/// Perform a ROC block write
	/// </summary>
	/// <param name="link">Link of the ROC to write</param>
	/// <param name="address">Address of the block</param>
	/// <param name="blockData">Vector of words to write</param>
	/// <param name="requestAck">Whether to request acknowledement of this operation</param>
	/// <param name="incrementAddress">Whether to increment the address pointer for block reads/writes</param>
	void WriteROCBlock(const DTC_Link_ID& link, const roc_address_t address, const std::vector<roc_data_t>& blockData, bool requestAck, bool incrementAddress);

	/// <summary>
	/// Sends a DCS Request Packet with fields filled in such that the given ROC firmware block register will be read out.
	/// This funcion reads from firmware blocks' register spaces.
	/// </summary>
	/// <param name="link">Link of the ROC to read</param>
	/// <param name="block">Block ID to read from</param>
	/// <param name="address">Address of the register</param>
	/// <returns>Value of the ROC register from a DCS Reply packet</returns>
	uint16_t ReadExtROCRegister(const DTC_Link_ID& link, const roc_address_t block, const roc_address_t address);
	/// <summary>
	/// Sends a DCS Request Packet with fields filled in such that the given ROC firmware block register will be written.
	/// This funcion writes to firmware blocks' register spaces.
	/// </summary>
	/// <param name="link">Link of the ROC to write to</param>
	/// <param name="block">Block ID to write to</param>
	/// <param name="address">Address of the register</param>
	/// <param name="data">Value to write</param>
	/// <param name="requestAck">Whether to request acknowledement of this operation</param>
	void WriteExtROCRegister(const DTC_Link_ID& link, const roc_address_t block, const roc_address_t address, const roc_data_t data, bool requestAck);
	/// <summary>
	/// Dump all known registers from the given ROC, via DCS Request packets.
	/// </summary>
	/// <param name="link">Link of the ROC</param>
	/// <returns>JSON-formatted register dump</returns>
	std::string ROCRegDump(const DTC_Link_ID& link);

	// Broadcast Readout
	/// <summary>
	/// DEPRECATED
	/// Sends a Readout Request broadcast to given Link.
	/// </summary>
	/// <param name="link">Link to send to</param>
	/// <param name="when">Timestamp for the Readout Request</param>
	/// <param name="quiet">Whether to not print the JSON representation of the Readout Request (Default: true, no JSON
	/// printed)</param>
	void SendReadoutRequestPacket(const DTC_Link_ID& link, const DTC_Timestamp& when, bool quiet = true);
	/// <summary>
	/// Send a DCS Request Packet to the given ROC. Use the Read/Write ROC Register functions for more convinient register
	/// access.
	/// </summary>
	/// <param name="link">Link of the ROC</param>
	/// <param name="type">Operation to perform</param>
	/// <param name="address">Target address</param>
	/// <param name="data">Data to write, if operation is write</param>
	/// <param name="address2">Second Target address</param>
	/// <param name="data2">Data to write to second address, if operation is write</param>
	/// <param name="quiet">Whether to not print the JSON representation of the Readout Request (Default: true, no JSON
	/// printed)</param>
	/// <param name="requestAck">Whether to request acknowledement of this operation</param>
	void SendDCSRequestPacket(const DTC_Link_ID& link, const DTC_DCSOperationType type, const roc_address_t address,
							  const roc_data_t data = 0x0, const roc_address_t address2 = 0x0, const roc_data_t data2 = 0,
							  bool quiet = true, bool requestAck = false);

	/// <summary>
	/// Writes a packet to the DTC on the DCS channel
	/// </summary>
	/// <param name="packet">Packet to write</param>
	void WriteDMAPacket(const DTC_DMAPacket& packet);
	/// <summary>
	/// Writes the given data buffer to the DTC's DDR memory, via the DAQ channel.
	/// </summary>
	/// <param name="buf">DMA buffer to write. Must have an inclusive 64-bit byte count at the beginning, followed by an
	/// exclusive 64-bit block count.</param> <param name="sz">Size ofthe data in the buffer</param>
	void WriteDetectorEmulatorData(mu2e_databuff_t* buf, size_t sz);
	/// <summary>
	/// Reads the next available DataHeaderPacket. If no data is available in the cache, a new DMA buffer is obtained.
	/// Performs several validity checks on the data, if no data is available or data is invalid, will return nullptr.
	/// </summary>
	/// <param name="tmo_ms">Timeout before returning nullptr. Default: 0, no timeout</param>
	/// <returns>Pointer to DataHeaderPacket. Will be nullptr if no data available.</returns>
	std::unique_ptr<DTC_DataHeaderPacket> ReadNextDAQPacket(int tmo_ms = 0);
	/// <summary>
	/// DCS packets are read one-at-a-time, this function reads the next one from the DTC
	/// </summary>
	/// <returns>Pointer to read DCSReplyPacket. Will be nullptr if no data available.</returns>
	std::unique_ptr<DTC_DCSReplyPacket> ReadNextDCSPacket(int tmo_ms = 0);

	/// <summary>
	/// Releases all buffers to the hardware, from both the DAQ and DCS channels
	/// </summary>
	void ReleaseAllBuffers()
	{
		ReleaseAllBuffers(DTC_DMA_Engine_DAQ);
		ReleaseAllBuffers(DTC_DMA_Engine_DCS);
	}

	/// <summary>
	/// Release all buffers to the hardware on the given channel
	/// </summary>
	/// <param name="channel">Channel to release</param>
	void ReleaseAllBuffers(const DTC_DMA_Engine& channel)
	{
		if (channel == DTC_DMA_Engine_DAQ)
			daqDMAInfo_.buffer.clear();
		else if (channel == DTC_DMA_Engine_DCS)
			dcsDMAInfo_.buffer.clear();
		device_.release_all(channel);
	}

private:
	std::unique_ptr<DTC_DataPacket> ReadNextPacket(const DTC_DMA_Engine& channel, int tmo_ms = 0);
	int ReadBuffer(const DTC_DMA_Engine& channel, int tmo_ms = 0, const int retries = 1);
	/// <summary>
	/// This function releases all buffers except for the one containing currentReadPtr. Should only be called when done
	/// with data in other buffers!
	/// </summary>
	/// <param name="channel">Channel to release</param>
	void ReleaseBuffers(const DTC_DMA_Engine& channel);
	void WriteDataPacket(const DTC_DataPacket& packet);

	struct DMAInfo
	{
		std::deque<mu2e_databuff_t*> buffer;
		uint32_t bufferIndex;
		void* currentReadPtr;
		void* lastReadPtr;
		DMAInfo()
			: buffer(), bufferIndex(0), currentReadPtr(nullptr), lastReadPtr(nullptr) {}
		~DMAInfo()
		{
			buffer.clear();
			currentReadPtr = nullptr;
			lastReadPtr = nullptr;
		}
	};
	int GetCurrentBuffer(DMAInfo* info);
	uint16_t GetBufferByteCount(DMAInfo* info, size_t index);
	DMAInfo daqDMAInfo_;
	DMAInfo dcsDMAInfo_;
};
}  // namespace DTCLib
#endif
