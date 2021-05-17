// This file (mu2edev.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb 13, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

/*
 *    make mu2edev.o CFLAGS='-g -Wall -std=c++0x'
 */

#include <signal.h>
#include <chrono>

#include "TRACE/tracemf.h"

#include "mu2edev.h"

mu2edev::mu2edev()
	: devfd_(0), buffers_held_(0), simulator_(nullptr), activeDTC_(0), deviceTime_(0LL), writeSize_(0), readSize_(0)
{
	// TRACE_CNTL( "lvlmskM", 0x3 );
	// TRACE_CNTL( "lvlmskS", 0x3 );
}

mu2edev::~mu2edev() { delete simulator_; }

int mu2edev::init(DTCLib::DTC_SimMode simMode, int dtc, std::string simMemoryFileName)
{
	auto start = std::chrono::steady_clock::now();
	if (simMode != DTCLib::DTC_SimMode_Disabled && simMode != DTCLib::DTC_SimMode_NoCFO &&
		simMode != DTCLib::DTC_SimMode_ROCEmulator && simMode != DTCLib::DTC_SimMode_Loopback)
	{
		simulator_ = new mu2esim(simMemoryFileName);
		simulator_->init(simMode);
	}
	else
	{
		if (simulator_ != nullptr)
		{
			delete simulator_;
			simulator_ = nullptr;
		}

		activeDTC_ = dtc;
		char devfile[11];
		snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, activeDTC_);
		int sts;
		devfd_ = open(devfile, O_RDWR);
		if (devfd_ == -1 || devfd_ == 0)
		{
			perror(("open " + std::string(devfile)).c_str());
			TRACE(TLVL_WARNING, "mu2e Device file not found and DTCLIB_SIM_ENABLE not set! Exiting.");
			throw std::runtime_error("mu2e Device file not found and DTCLIB_SIM_ENABLE not set! Exiting.");
			//exit(1);
		}
		for (unsigned chn = 0; chn < MU2E_MAX_CHANNELS; ++chn)
			for (unsigned dir = 0; dir < 2; ++dir)
			{
				m_ioc_get_info_t get_info;
				get_info.chn = chn;
				get_info.dir = dir;
				get_info.tmo_ms = 0;
				TRACE(TLVL_DEBUG + 10, "mu2edev::init before ioctl( devfd_, M_IOC_GET_INFO, &get_info ) chn=%u dir=%u", chn, dir);
				sts = ioctl(devfd_, M_IOC_GET_INFO, &get_info);
				if (sts != 0)
				{
					perror("M_IOC_GET_INFO");

					throw std::runtime_error("Failed mu2edev::init before ioctl( devfd_, M_IOC_GET_INFO, &get_info)");
					//exit(1);
				}
				mu2e_channel_info_[activeDTC_][chn][dir] = get_info;
				TRACE(TLVL_DEBUG, "mu2edev::init %d %u:%u - num=%u size=%u hwIdx=%u, swIdx=%u delta=%u", activeDTC_, chn, dir,
					  get_info.num_buffs, get_info.buff_size, get_info.hwIdx, get_info.swIdx,
					  mu2e_chn_info_delta_(activeDTC_, chn, dir, &mu2e_channel_info_));
				for (unsigned map = 0; map < 2; ++map)
				{
					size_t length = get_info.num_buffs * ((map == MU2E_MAP_BUFF) ? get_info.buff_size : sizeof(int));
					// int prot = (((dir == S2C) && (map == MU2E_MAP_BUFF))? PROT_WRITE : PROT_READ);
					int prot = (((map == MU2E_MAP_BUFF)) ? PROT_WRITE : PROT_READ);
					off64_t offset = chnDirMap2offset(chn, dir, map);
					mu2e_mmap_ptrs_[activeDTC_][chn][dir][map] = mmap(0 /* hint address */
																	  ,
																	  length, prot, MAP_SHARED, devfd_, offset);
					if (mu2e_mmap_ptrs_[activeDTC_][chn][dir][map] == MAP_FAILED)
					{
						perror("mmap");
						throw std::runtime_error("mmap");
						//exit(1);
					}
					TRACE(TLVL_DEBUG, "mu2edev::init chnDirMap2offset=%lu mu2e_mmap_ptrs_[%d][%d][%d][%d]=%p p=%c l=%lu", offset, dtc, chn,
						  dir, map, mu2e_mmap_ptrs_[activeDTC_][chn][dir][map], prot == PROT_READ ? 'R' : 'W', length);
				}
				if (dir == DTC_DMA_Direction_C2S)
				{
					release_all(static_cast<DTC_DMA_Engine>(chn));
				}

				// Reset the DTC
				//{
				//	write_register(0x9100, 0, 0xa0000000);
				//	write_register(0x9118, 0, 0x0000003f);
				//	write_register(0x9100, 0, 0x00000000);
				//	write_register(0x9100, 0, 0x10000000);
				//	write_register(0x9100, 0, 0x30000000);
				//	write_register(0x9100, 0, 0x10000000);
				//	write_register(0x9118, 0, 0x00000000);
				//}

				// Enable DMA Engines
				{
					// uint16_t addr = DTC_Register_Engine_Control(chn, dir);
					// TRACE(17, "mu2edev::init write Engine_Control reg 0x%x", addr);
					// write_register(addr, 0, 0x100);//bit 8 enable=1
				}
			}
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	return simMode;
}

/*****************************
   read_data
   returns number of bytes read; negative value indicates an error
   */
int mu2edev::read_data(DTC_DMA_Engine const& chn, void** buffer, int tmo_ms)
{
	auto start = std::chrono::steady_clock::now();
	auto retsts = -1;
	if (simulator_ != nullptr)
	{
		retsts = simulator_->read_data(chn, buffer, tmo_ms);
	}
	else
	{
		retsts = 0;
		unsigned has_recv_data;
		TRACE(TLVL_DEBUG + 11, "mu2edev::read_data before (mu2e_mmap_ptrs_[%d][0][0][0]!=NULL) || ((retsts=init())==0)", activeDTC_);
		if ((mu2e_mmap_ptrs_[activeDTC_][0][0][0] != NULL) ||
			((retsts = init(DTCLib::DTC_SimMode_Disabled, 0)) == 0))  // Default-init mu2edev if not given guidance
		{
			has_recv_data = mu2e_chn_info_delta_(activeDTC_, chn, C2S, &mu2e_channel_info_);
			TRACE(TLVL_DEBUG + 11, "mu2edev::read_data after %u=has_recv_data = delta_( chn, C2S )", has_recv_data);
			mu2e_channel_info_[activeDTC_][chn][C2S].tmo_ms = tmo_ms;  // in case GET_INFO is called
			if ((has_recv_data > buffers_held_) ||
				((retsts = ioctl(devfd_, M_IOC_GET_INFO, &mu2e_channel_info_[activeDTC_][chn][C2S])) == 0 &&
				 (has_recv_data = mu2e_chn_info_delta_(activeDTC_, chn, C2S, &mu2e_channel_info_)) >
					 buffers_held_))
			{  // have data
				// get byte count from new/next
				unsigned newNxtIdx =
					idx_add(mu2e_channel_info_[activeDTC_][chn][C2S].swIdx, (int)buffers_held_ + 1, activeDTC_, chn, C2S);
				int* BC_p = (int*)mu2e_mmap_ptrs_[activeDTC_][chn][C2S][MU2E_MAP_META];
				retsts = BC_p[newNxtIdx];
				*buffer = ((mu2e_databuff_t*)(mu2e_mmap_ptrs_[activeDTC_][chn][C2S][MU2E_MAP_BUFF]))[newNxtIdx];
				TRACE(TLVL_INFO,
					  "mu2edev::read_data chn%d hIdx=%u, sIdx=%u "
					  "%u hasRcvDat=%u %p[newNxtIdx=%d]=retsts=%d buf(%p)[0]=0x%08x",
					  chn, mu2e_channel_info_[activeDTC_][chn][C2S].hwIdx, mu2e_channel_info_[activeDTC_][chn][C2S].swIdx,
					  mu2e_channel_info_[activeDTC_][chn][C2S].num_buffs, has_recv_data, (void*)BC_p, newNxtIdx, retsts,
					  *buffer, *(uint32_t*)*buffer);
				++buffers_held_;
			}
			else
			{  // was it a tmo or error
				if (retsts != 0)
				{
					perror("M_IOC_GET_INFO");
					exit(1);
				}
				TRACE(TLVL_DEBUG + 11, "mu2edev::read_data not error... return 0 status");
			}
		}
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	if (retsts > 0) readSize_ += retsts;
	return retsts;
}  // read_data

/* read_release
   release a number of buffers (usually 1)
   */
int mu2edev::read_release(DTC_DMA_Engine const& chn, unsigned num)
{
	auto start = std::chrono::steady_clock::now();
	auto retsts = -1;
	if (simulator_ != nullptr)
	{
		retsts = simulator_->read_release(chn, num);
	}
	else
	{
		retsts = 0;
		unsigned long arg;
		unsigned has_recv_data;
		has_recv_data = mu2e_chn_info_delta_(activeDTC_, chn, C2S, &mu2e_channel_info_);
		if (num <= has_recv_data)
		{
			arg = (chn << 24) | (C2S << 16) | (num & 0xffff);  // THIS OBIVOUSLY SHOULD BE A MACRO
			retsts = ioctl(devfd_, M_IOC_BUF_GIVE, arg);
			if (retsts != 0)
			{
				perror("M_IOC_BUF_GIVE");
			}  // exit(1); } // Don't exit for now

			// increment our cached info
			mu2e_channel_info_[activeDTC_][chn][C2S].swIdx =
				idx_add(mu2e_channel_info_[activeDTC_][chn][C2S].swIdx, (int)num, activeDTC_, chn, C2S);
			if (num <= buffers_held_)
				buffers_held_ -= num;
			else
				buffers_held_ = 0;
		}
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	return retsts;
}

int mu2edev::read_register(uint16_t address, int tmo_ms, uint32_t* output)
{
	auto start = std::chrono::steady_clock::now();
	if (simulator_ != nullptr)
	{
		return simulator_->read_register(address, tmo_ms, output);
	}
	m_ioc_reg_access_t reg;
	reg.reg_offset = address;
	reg.access_type = 0;

	int counter = 0;
	int errorCode = -99;

	while (counter < 5 && errorCode < 0)
	{
		errorCode = ioctl(devfd_, M_IOC_REG_ACCESS, &reg);
		counter++;
		if (errorCode < 0) usleep(10000);
	}
	*output = reg.val;
	TRACE(TLVL_DEBUG + 15, "Read value 0x%x from register 0x%x errorcode %d", reg.val, address, errorCode);
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	return errorCode;
}

int mu2edev::write_register(uint16_t address, int tmo_ms, uint32_t data)
{
	auto start = std::chrono::steady_clock::now();
	auto retsts = -1;
	if (simulator_ != nullptr)
	{
		retsts = simulator_->write_register(address, tmo_ms, data);
	}
	else
	{
		m_ioc_reg_access_t reg;
		reg.reg_offset = address;
		reg.access_type = 1;
		reg.val = data;
		TRACE(TLVL_DEBUG + 16, "Writing value 0x%x to register 0x%x", data, address);
		retsts = ioctl(devfd_, M_IOC_REG_ACCESS, &reg);
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	return retsts;
}

void mu2edev::meta_dump()
{
	TRACE(TLVL_DEBUG + 5, "mu2edev::meta_dump");
	auto start = std::chrono::steady_clock::now();
	if (simulator_ == nullptr)
	{
		int retsts = 0;
		for (int chn = 0; chn < MU2E_MAX_CHANNELS; ++chn)
			for (int dir = 0; dir < 2; ++dir)
				if ((mu2e_mmap_ptrs_[activeDTC_][0][0][0] != NULL) ||
					((retsts = init(DTCLib::DTC_SimMode_Disabled, 0)) == 0))  // Default-init mu2edev if not given guidance
				{
					for (unsigned buf = 0; buf < mu2e_channel_info_[activeDTC_][chn][dir].num_buffs; ++buf)
					{
						int* BC_p = (int*)mu2e_mmap_ptrs_[activeDTC_][chn][dir][MU2E_MAP_META];
						printf("buf_%02d: %u\n", buf, BC_p[buf]);
					}
				}
		retsts = ioctl(devfd_, M_IOC_DUMP);
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
}

int mu2edev::write_data(DTC_DMA_Engine const& chn, void* buffer, size_t bytes)
{
	auto start = std::chrono::steady_clock::now();
	auto retsts = -1;
	if (simulator_ != nullptr)
	{
		retsts = simulator_->write_data(chn, buffer, bytes);
	}
	else
	{
		int dir = S2C;
		retsts = 0;
		unsigned delta = mu2e_chn_info_delta_(activeDTC_, chn, dir, &mu2e_channel_info_);  // check cached info
		TRACE(TLVL_INFO, "write_data delta=%u chn=%d dir=S2C, sz=%zu", delta, chn, bytes);
		while (delta <= 1 &&
			   std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() <
				   1000)
		{
			m_ioc_get_info_t get_info;
			get_info.chn = chn;
			get_info.dir = dir;
			get_info.tmo_ms = 0;
			int sts = ioctl(devfd_, M_IOC_GET_INFO, &get_info);
			if (sts != 0)
			{
				perror("M_IOC_GET_INFO");
				exit(1);
			}
			mu2e_channel_info_[activeDTC_][chn][dir] = get_info;  // copy info struct
			delta = mu2e_chn_info_delta_(activeDTC_, chn, dir, &mu2e_channel_info_);
			usleep(1000);
		}

		if (delta <= 1)
		{
			TRACE(TLVL_ERROR, "HW_NOT_READING_BUFS");
			perror("HW_NOT_READING_BUFS");
			kill(0, SIGUSR2);
			exit(2);
		}

		unsigned idx = mu2e_channel_info_[activeDTC_][chn][dir].swIdx;
		void* data = ((mu2e_databuff_t*)(mu2e_mmap_ptrs_[activeDTC_][chn][dir][MU2E_MAP_BUFF]))[idx];
		memcpy(data, buffer, bytes);
		unsigned long arg = (chn << 24) | (bytes & 0xffffff);  // THIS OBIVOUSLY SHOULD BE A MACRO

		int retry = 15;
		do
		{
			retsts = ioctl(devfd_, M_IOC_BUF_XMIT, arg);
			if (retsts != 0)
			{
				TRACE(TLVL_INFO, "write_data ioctl returned %d, errno=%d (%s), retrying.", retsts, errno, strerror(errno));
				// perror("M_IOC_BUF_XMIT");
				usleep(50000);
			}  // exit(1); } // Take out the exit call for now
			retry--;
		} while (retry > 0 && retsts != 0);
		// increment our cached info
		if (retsts == 0)
		{
			mu2e_channel_info_[activeDTC_][chn][dir].swIdx =
				idx_add(mu2e_channel_info_[activeDTC_][chn][dir].swIdx, 1, activeDTC_, chn, dir);
		}
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	if (retsts >= 0) writeSize_ += bytes;
	return retsts;
}  // write_data

// applicable for recv.
int mu2edev::release_all(DTC_DMA_Engine const& chn)
{
	auto start = std::chrono::steady_clock::now();
	auto retsts = 0;
	if (simulator_ != nullptr)
	{
		retsts = simulator_->release_all(chn);
	}
	else
	{
		auto has_recv_data = mu2e_chn_info_delta_(activeDTC_, chn, C2S, &mu2e_channel_info_);
		if (has_recv_data) read_release(chn, has_recv_data);
	}
	deviceTime_ += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
	return retsts;
}

void mu2edev::close()
{
	if (simulator_ != nullptr)
	{
		delete simulator_;
		simulator_ = nullptr;
	}
}
