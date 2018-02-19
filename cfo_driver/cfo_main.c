/*  This file (cfo.c) was created by Ron Rechenmacher <ron@fnal.gov> on
	Feb  5, 2014. "TERMS AND CONDITIONS" governing this file are in the README
	or COPYING file. If you do not have such a file, one can be obtained by
	contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
	$RCSfile: .emacs.gnu,v $
	rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";
	*/
#include <linux/module.h>	// module_param, THIS_MODULE
#include <linux/init.h>		// module_init,_exit
#include <linux/kernel.h>	// KERN_INFO, printk
#include <linux/fs.h>		/* struct inode */
#include <linux/pci.h>          /* struct pci_dev *pci_get_device */
#include <linux/delay.h>	/* msleep */
#include <linux/wait.h>		/* wait_event_interruptible_timeout */
#include <linux/jiffies.h>	/* msec_to_jiffies */
#include <linux/interrupt.h>	/* request_irq */
#include <asm/uaccess.h>	/* access_ok, copy_to_user */

#include "xdma_hw.h"		/* struct BuffDesc */

#include "trace.h"		/* TRACE */
#include "cfo_fs.h"		/* cfo_ioctl prototype */
#include "cfo_pci.h"		/* bar_info_t, extern cfo_pci*  */
#include "cfo_event.h"
#include "cfo_mmap_ioctl.h"
#include "cfo_proto_globals.h"	/* CFO_MAX_CHANNEL, etc. */


	/* GLOBALS */

struct pci_dev *cfo_pci_dev = 0;

bar_info_t      cfo_pcie_bar_info;


pci_sender_t cfo_pci_sender[CFO_NUM_SEND_CHANNELS] = { {0}, };

pci_recver_t cfo_pci_recver[CFO_NUM_RECV_CHANNELS] = { {0}, };

// This variable name is used in a macro that expects the same
// variable name in the user-space "library"
m_ioc_get_info_t cfo_channel_info_[CFO_MAX_CHANNELS][2]; // See enums in cfo_mmap_ioctl.h (0=C2S)

//    ch,dir,buffers/meta
volatile void *  cfo_mmap_ptrs[CFO_MAX_CHANNELS][2][2];

/* for exclusion of all program flows (processes, ISRs and BHs) */
static DEFINE_SPINLOCK(DmaStatsLock);

/**
 * The get_info_wait_queue allows this module to put
 * userspace processes that are reading data to sleep
 * if there is no data available.
 */
DECLARE_WAIT_QUEUE_HEAD(get_info_wait_queue);

#define MAX_STATS   100
/* Statistics-related variables */
DMAStatistics DStats[MAX_DMA_ENGINES][MAX_STATS];
SWStatistics SStats[MAX_DMA_ENGINES][MAX_STATS];
TRNStatistics TStats[MAX_STATS];
int dstatsRead[MAX_DMA_ENGINES], dstatsWrite[MAX_DMA_ENGINES];
int dstatsNum[MAX_DMA_ENGINES], sstatsRead[MAX_DMA_ENGINES];
int sstatsWrite[MAX_DMA_ENGINES], sstatsNum[MAX_DMA_ENGINES];
int tstatsRead, tstatsWrite, tstatsNum;
u32 SWrate[MAX_DMA_ENGINES];
int MSIEnabled = 0;


//////////////////////////////////////////////////////////////////////////////
/* forward decl */
static int ReadPCIState(struct pci_dev * pdev, m_ioc_pcistate_t * pcistate);

static int checkDmaEngine(unsigned chn, unsigned dir) {
	int sts = 0;
	u32 status = Dma_mReadChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS);
	int lc = 5;

	if (dir == C2S && (status & (DMA_ENG_INT_ALERR | DMA_ENG_INT_FETERR | DMA_ENG_INT_ABORTERR | DMA_ENG_INT_CHAINEND)) != 0)
	{
		TRACE(20, "checkDmaEngine: One of the error bits set: chn=%d dir=%d sts=0x%llx", chn, dir, (unsigned long long)status);
		printk("CFO DMA Interrupt Error Bits Set: chn=%d dir=%d, sts=0x%llx", chn, dir, (unsigned long long)status);
		/* Perform soft reset of DMA engine */
		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_USER_RESET);
		status = Dma_mReadChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS);
		while ((status & DMA_ENG_USER_RESET) != 0 && lc > 0) { status = Dma_mReadChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS); --lc; }
		lc = 5;
		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_RESET);
		while ((status & DMA_ENG_RESET) != 0 && lc > 0) { status = Dma_mReadChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS); --lc; }
		sts = 1;
	}

	if ((status & DMA_ENG_ENABLE) == 0)
	{
		TRACE(20, "checkDmaEngine: DMA ENGINE DISABLED! Re-enabling... chn=%d dir=%d", chn, dir);
		if (dir == C2S) {
			Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_ENABLE | DMA_ENG_INT_ENABLE);
		}
		else
		{
			Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_ENABLE);
		}
		sts = 1;
	}

	if ((status & DMA_ENG_STATE_MASK) != 0)
	{
		TRACE(20, "checkDmaEngine: DMA Engine Status: chn=%d dir=%d r=%d, w=%d", chn, dir, ((status & DMA_ENG_RUNNING) != 0 ? 1 : 0), ((status & DMA_ENG_WAITING) != 0 ? 1 : 0));
	}
	return sts;
}

static irqreturn_t DmaInterrupt(int irq, void *dev_id)
{
#if CFO_RECV_INTER_ENABLED
	unsigned long base;
	unsigned chn;

	TRACE(20, "DmaInterrrupt: start");

	base = (unsigned long)(cfo_pcie_bar_info.baseVAddr);
	Dma_mIntDisable(base);

	TRACE(20, "DmaInterrupt: Checking DMA Engines");
	/* Check interrupt for error conditions */
	for (chn = 0; chn < 2; ++chn) {
		checkDmaEngine(chn, C2S);
	}

	TRACE(20, "DmaInterrupt: Calling poll routine");
	/* Handle DMA and any user interrupts */
#if 0
	if (cfo_sched_poll() == 0)
#else
	if (cfo_force_poll() == 0)
#endif
	{
		Dma_mIntAck(base, DMA_ENG_ALLINT_MASK);
		return IRQ_HANDLED;
	}
	else
#endif
		return IRQ_NONE;
}


int cfo_mmap(struct file *file, struct vm_area_struct *vma)
{
	int	          ch, dir, map;
	unsigned long phys_addr, uaddr;
	int           sts = 0, ii;

	page2chDirMap(vma->vm_pgoff, ch, dir, map);
	TRACE(4, "cfo_mmap: vm_pgoff:%lu ch:%d dir:%d map:%d: %p"
		, vma->vm_pgoff, ch, dir, map, cfo_mmap_ptrs[ch][dir][map]);
	if (map == CFO_MAP_META)
		vma->vm_flags &= ~VM_WRITE;

	if (dir == C2S && map == CFO_MAP_BUFF) {
		uaddr = vma->vm_start;
		for (ii = 0; ii < CFO_NUM_RECV_BUFFS; ++ii) {
			phys_addr = virt_to_phys(((void**)cfo_mmap_ptrs[ch][dir][map])[ii]);
			sts |= io_remap_pfn_range(vma, uaddr
				, phys_addr >> PAGE_SHIFT
				, sizeof(cfo_databuff_t)
				, vma->vm_page_prot);
			uaddr += sizeof(cfo_databuff_t);
		}
	}
	else
	{
		phys_addr = virt_to_phys(cfo_mmap_ptrs[ch][dir][map]);
		sts = io_remap_pfn_range(vma, vma->vm_start
			, phys_addr >> PAGE_SHIFT
			, vma->vm_end - vma->vm_start
			, vma->vm_page_prot);
	}
	if (sts) return -EAGAIN;

	return (0);
}   // cfo_mmap

IOCTL_RET_TYPE cfo_ioctl(IOCTL_ARGS(struct inode *inode, struct file *filp
	, unsigned int cmd, unsigned long arg))
{
	IOCTL_RET_TYPE		retval = 0;
	unsigned long		base;
	unsigned		jj;
	m_ioc_reg_access_t	reg_access;
	m_ioc_get_info_t	get_info;
	int			chn, dir, num;
	unsigned		myIdx, nxtIdx, hwIdx;
	volatile cfo_buffdesc_S2C_t   *desc_S2C_p;
	u32                 	descDmaAdr_swNxt;
	m_ioc_pcistate_t	pcistate;
	m_ioc_engstate_t	eng;
	m_ioc_engstats_t	es;
	TRNStatsArray		tsa;
	int			which_engine, len, ii;
	DMAStatistics	       *ds;
	TRNStatistics	       *ts;
	unsigned		tmo_jiffies;

	TRACE(11, "cfo_ioctl: start - cmd=0x%x", cmd);
	if (_IOC_TYPE(cmd) != CFO_IOC_MAGIC) return -ENOTTY;

	/* Check read/write and corresponding argument */
	if (_IOC_DIR(cmd) & _IOC_READ)
		if (!access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		if (!access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd)))
			return -EFAULT;

	/* DMA registers are offset from BAR0 */
	base = (unsigned long)(cfo_pcie_bar_info.baseVAddr);

	TRACE(11, "cfo_ioctl: start2");
	switch (cmd)
	{
	case M_IOC_GET_TST_STATE:
		TRACE(11, "cfo_ioctl: cmd=GET_TST_STATE");
		break;
	case M_IOC_TEST_START:
		TRACE(11, "cfo_ioctl: cmd=TEST_START");
		// enable dma ch0/C2S w/GENERATOR
		Dma_mWriteChnReg(0, C2S, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_ENABLE);
		msleep(20);
		Dma_mWriteReg(cfo_pcie_bar_info.baseVAddr
			, 0x9108, /*0x3f*/0xffffffff); // LOOPBACK
		msleep(10);
		//Dma_mWriteReg( base, 0x9100, 1 );  // 1=enable generator
		break;
	case M_IOC_TEST_STOP:
		TRACE(11, "cfo_ioctl: cmd=TEST_STOP");
		break;
		/* ------------------------------------------------------------------- */

	case M_IOC_GET_PCI_STATE:	/* m_ioc_pcistate_t; formerly IGET_PCI_STATE      _IOR(XPMON_MAGIC,4,PCIState) */
		TRACE(11, "cfo_ioctl: cmd=GET_PCI_STATE");
		ReadPCIState(cfo_pci_dev, &pcistate);
		if (copy_to_user((m_ioc_pcistate_t *)arg, &pcistate, sizeof(m_ioc_pcistate_t)))
		{
			printk("copy_to_user failed\n");
			retval = -EFAULT;
			break;
		}
		break;
	case M_IOC_GET_ENG_STATE:	/* m_ioc_engstate_t; formerly IGET_ENG_STATE      _IOR(XPMON_MAGIC,5,EngState) */
		TRACE(11, "cfo_ioctl: cmd=GET_ENG_STATE");
		if (copy_from_user(&eng, (m_ioc_engstate_t *)arg, sizeof(m_ioc_engstate_t)))
		{
			printk("\ncopy_from_user failed\n");
			retval = -EFAULT;
			break;
		}

		which_engine = eng.Engine;  //printk("For engine %d\n", i);

		/* First, check if requested engine is valid */
		if ((which_engine >= MAX_DMA_ENGINES) /*|| (!((dmaData->engineMask) & (1LL << i)))*/)
		{
			printk("Invalid engine %d\n", which_engine);
			retval = -EFAULT;
			break;
		}


		/* First, get the user state */
		eng.Buffers = 4;      //ustate.Buffers;
		eng.MinPktSize = 64;     //ustate.MinPktSize;
		eng.MaxPktSize = 0x8000; //ustate.MaxPktSize;
		eng.TestMode = 1;      //ustate.TestMode;

			/* Now add the DMA state */
		eng.BDs = 399; /* FNAL devel -- linked to sguser.c:#define NUM_BUFS  and DmaSetupTransmit(handle[0],100) ??? */
		eng.BDerrs = 0;        //rptr->BDerrs;
		eng.BDSerrs = 0;        //rptr->BDSerrs;
#      ifdef TH_BH_ISR
		eng.IntEnab = 1;
#      else
		eng.IntEnab = 0;
#      endif
		if (copy_to_user((m_ioc_engstate_t *)arg, &eng, sizeof(m_ioc_engstate_t)))
		{
			printk("copy_to_user failed\n");
			retval = -EFAULT;
			break;
		}
		break;
	case M_IOC_GET_DMA_STATS:	/* m_ioc_engstats_t; formerly IGET_DMA_STATISTICS _IOR(XPMON_MAGIC,6,EngStatsArray) */
		TRACE(11, "cfo_ioctl: cmd=GET_DMA_STATS");
		if (copy_from_user(&es, (m_ioc_engstats_t *)arg, sizeof(m_ioc_engstats_t)))
		{
			printk("copy_from_user failed\n");
			retval = -1;
			break;
		}

		ds = es.engptr;
		len = 0;
		for (ii = 0; ii < es.Count; ++ii)
		{
			DMAStatistics from;
			int j;

			/* Must copy in a round-robin manner so that reporting is fair */
			for (j = 0; j < MAX_DMA_ENGINES; j++)
			{
				if (!dstatsNum[j]) continue;

				spin_lock_bh(&DmaStatsLock);
				from = DStats[j][dstatsRead[j]];
				from.Engine = j;
				dstatsNum[j] -= 1;
				dstatsRead[j] += 1;
				if (dstatsRead[j] == MAX_STATS)
					dstatsRead[j] = 0;
				spin_unlock_bh(&DmaStatsLock);

				if (copy_to_user(ds, &from, sizeof(DMAStatistics)))
				{
					printk("copy_to_user failed\n");
					retval = -EFAULT;
					break;
				}

				len++;
				ii++;
				if (ii >= es.Count) break;
				ds++;
			}
			if (retval < 0) break;
		}
		es.Count = len;
		if (copy_to_user((m_ioc_engstats_t *)arg, &es, sizeof(m_ioc_engstats_t)))
		{
			printk("copy_to_user failed\n");
			retval = -EFAULT;
			break;
		}
		break;
	case M_IOC_GET_TRN_STATS:   /* TRNStatsArray;    formerly IGET_TRN_STATISTICS _IOR(XPMON_MAGIC,7,TRNStatsArray) */
		TRACE(11, "cfo_ioctl: cmd=GET_TRN_STATS");
		if (copy_from_user(&tsa, (TRNStatsArray *)arg, sizeof(TRNStatsArray)))
		{
			printk("copy_from_user failed\n");
			retval = -1;
			break;
		}

		ts = tsa.trnptr;
		len = 0;
		for (ii = 0; ii < tsa.Count; ++ii)
		{
			TRNStatistics from;

			if (!tstatsNum) break;

			spin_lock_bh(&DmaStatsLock);
			from = TStats[tstatsRead];
			tstatsNum -= 1;
			tstatsRead += 1;
			if (tstatsRead == MAX_STATS)
				tstatsRead = 0;
			spin_unlock_bh(&DmaStatsLock);

			if (copy_to_user(ts, &from, sizeof(TRNStatistics)))
			{
				printk("copy_to_user failed\n");
				retval = -EFAULT;
				break;
			}

			len++;
			ts++;
		}
		tsa.Count = len;
		if (copy_to_user((TRNStatsArray *)arg, &tsa, sizeof(TRNStatsArray)))
		{
			printk("copy_to_user failed\n");
			retval = -EFAULT;
			break;
		}
		break;

		/* ------------------------------------------------------------------- */
	case M_IOC_REG_ACCESS:

		if (copy_from_user(&reg_access, (void*)arg, sizeof(reg_access)))
		{
			printk("copy_from_user failed\n"); return (-EFAULT);
		}
		if (reg_access.access_type)
		{
			TRACE(11, "cfo_ioctl: cmd=REG_ACCESS - write offset=0x%x, val=0x%x", reg_access.reg_offset, reg_access.val);
			Dma_mWriteReg(base, reg_access.reg_offset, reg_access.val);
		}
		else
		{
			TRACE(11, "cfo_ioctl: cmd=REG_ACCESS - read offset=0x%x", reg_access.reg_offset);
			reg_access.val = Dma_mReadReg(base, reg_access.reg_offset);
			if (copy_to_user((void*)arg, &reg_access, sizeof(reg_access)))
			{
				printk("copy_to_user failed\n"); return (-EFAULT);
			}
		}
		break;
	case M_IOC_GET_INFO:
		if (copy_from_user(&get_info, (void*)arg, sizeof(m_ioc_get_info_t))) {
			TRACE( 0,"copy_from_user failed\n"); return (-EFAULT);
		}
		tmo_jiffies = msecs_to_jiffies(get_info.tmo_ms);
		dir = get_info.dir;
		chn = get_info.chn;
		if (get_info.dir == C2S) {
			if (!cfo_chn_info_delta_(get_info.chn, C2S, &cfo_channel_info_)) {
				TRACE( 11, "cfo_ioctl: cmd=GET_INFO wait_event_interruptible_timeout jiffies=%u", tmo_jiffies);
				if (wait_event_interruptible_timeout(get_info_wait_queue
				                                     , cfo_chn_info_delta_(get_info.chn, C2S, &cfo_channel_info_)
				                                     , tmo_jiffies) == 0) {
					TRACE(16, "cfo_ioctl: cmd=GET_INFO tmo");
				}
			}
		} else {
			jj=cfo_channel_info_[chn][dir].num_buffs;
			hwIdx = cfo_channel_info_[chn][dir].hwIdx;
			while ( ((cfo_buffdesc_S2C_t*)idx2descVirtAdr(hwIdx,chn,dir))->Complete
			       && hwIdx != cfo_channel_info_[chn][dir].swIdx
			       && jj-- ) {
				hwIdx = idx_add(hwIdx, 1, chn, dir);
				TRACE( 11, "ioctl GET_INFO cfo_channel_info_[chn][dir].hwIdx=%u swIdx=%u lps=%u"
				      , hwIdx, cfo_channel_info_[chn][dir].swIdx, jj );
			}
			cfo_channel_info_[chn][dir].hwIdx = hwIdx;
		}
		get_info = cfo_channel_info_[get_info.chn][get_info.dir];
		TRACE( 11, "cfo_ioctl: cmd=GET_INFO dir=%d get_info.dir=%u hwIdx=%u swIdx=%u"
		      ,dir,get_info.dir, get_info.hwIdx, get_info.swIdx );
		if (copy_to_user((void*)arg, &get_info, sizeof(m_ioc_get_info_t)))
		{
			TRACE( 0,"copy_to_user failed\n"); return (-EFAULT);
		}
		break;
	case M_IOC_BUF_GIVE:
		TRACE(11, "cfo_ioctl: cmd=BUF_GIVE");
		chn = arg >> 24;
		dir = (arg >> 16) & 1;
		num = arg & 0xffff;
		TRACE(11, "cfo_ioctl: BUF_GIVE chn:%u dir:%u num:%u", chn, dir, num);
		myIdx = idx_add(cfo_channel_info_[chn][dir].swIdx, num, chn, dir);
		Dma_mWriteChnReg(chn, dir, REG_SW_NEXT_BD, idx2descDmaAdr(myIdx, chn, dir));
		checkDmaEngine(chn, dir);
		cfo_channel_info_[chn][dir].swIdx = myIdx;
		break;
	case M_IOC_DUMP:
		TRACE(10, "SERDES LOOPBACK Enable 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9108));
		TRACE(10, "Ring Enable 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9114));
		TRACE(10, "SERDES Rx Disparity error (2 bits/link) 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x911c));
		TRACE(10, "SERDES Rx character not in table (2 bits/link) 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9120));
		TRACE(10, "SERDES unlock error 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9124));
		TRACE(10, "SERDES PLL lock 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9128));
		TRACE(10, "SERDES Tx buffer status (2 bits/link) 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x912c));
		TRACE(10, "SERDES Rx buffer status (3 bits/link) 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9130));
		TRACE(10, "SERDES Reset done 0x%x"
			, Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9138));
		for (chn = 0; chn < 2; ++chn)
			for (dir = 0; dir < 2; ++dir)
			{
				u32 hw_next = Dma_mReadChnReg(chn, dir, REG_HW_NEXT_BD);
				u32 sw_next = Dma_mReadChnReg(chn, dir, REG_SW_NEXT_BD);
				u32 hw_cmplt = Dma_mReadChnReg(chn, dir, REG_HW_CMPLT_BD);
				TRACE(10, "chn=%d dir=%x (0x%08x) hw_next_idx=%u sw_next_idx=%u hw_cmplt_idx=%u"
					, chn, dir == 0 ? 0xC25 : 0x52C
					, hw_next
				      , descDmaAdr2idx(hw_next, chn, dir,0)
				      , descDmaAdr2idx(sw_next, chn, dir,0)
				      , descDmaAdr2idx(hw_cmplt, chn, dir,0));
				if (dir == 0) // C25
				{
					u32 sw_has_recv_data;
					u32 hw = descDmaAdr2idx(hw_next, chn, dir,0);
					u32 sw = descDmaAdr2idx(sw_next, chn, dir,0);
					u32 hw_has_recv_data = ((hw >= sw)
						? hw - sw
						: CFO_NUM_RECV_BUFFS + hw - sw);
					hw = cfo_channel_info_[chn][dir].hwIdx;
					sw = cfo_channel_info_[chn][dir].swIdx;
					sw_has_recv_data = ((hw >= sw)
						? hw - sw
						: CFO_NUM_RECV_BUFFS + hw - sw);
					TRACE(10, "hw_has_recv_data=%u sw=%u", hw_has_recv_data, sw_has_recv_data);
				}
			}
		TRACE(10, "RECV[0] BUFFS:");
		for (jj = 0; jj < CFO_NUM_RECV_BUFFS; ++jj)
		{
			TRACE(10, "%3u %2x 0x%08x", jj, 0
				, ((u32*)&(cfo_pci_recver[0].buffdesc_ring[jj]))[0]);
			TRACE(10, "%3u %2x 0x%016llx", jj, 4
				, cfo_pci_recver[0].buffdesc_ring[jj]->UserStatus);
			TRACE(10, "%3u %2x 0x%08x", jj, 12
				, cfo_pci_recver[0].buffdesc_ring[jj]->CardAddress);
			TRACE(10, "%3u %2x 0x%08x IrqComplete=%u", jj, 16
				, ((u32*)&(cfo_pci_recver[0].buffdesc_ring[jj]))[4]
				, cfo_pci_recver[0].buffdesc_ring[jj]->IrqComplete);
			TRACE(10, "%3u %2x 0x%016llx", jj, 20
				, cfo_pci_recver[0].buffdesc_ring[jj]->SystemAddress);
			TRACE(10, "%3u %2x 0x%08x", jj, 28
				, cfo_pci_recver[0].buffdesc_ring[jj]->NextDescPtr);
			TRACE(10, "%3u meta@%p[%d]=%u", jj
				, cfo_mmap_ptrs[0][C2S][CFO_MAP_META], jj
				, ((u32*)(cfo_mmap_ptrs[0][C2S][CFO_MAP_META]))[jj]);
			TRACE(10, "%3u 0x%08x 0x%08x 0x%08x 0x%08x", jj
				, ((u32*)&(cfo_pci_recver[0].databuffs[jj]))[0]
				, ((u32*)&(cfo_pci_recver[0].databuffs[jj]))[1]
				, ((u32*)&(cfo_pci_recver[0].databuffs[jj]))[2]
				, ((u32*)&(cfo_pci_recver[0].databuffs[jj]))[3]);
		}
		TRACE(10, "SEND[0] BUFFS:");
		for (jj = 0; jj < CFO_NUM_SEND_BUFFS; ++jj)
		{
			TRACE(10, "%3u %2x 0x%08x", jj, 0
				, ((u32*)&(cfo_pci_sender[0].buffdesc_ring[jj]))[0] );
			TRACE(10, "%3u %2x 0x%016llx", jj, 4
				, cfo_pci_sender[0].buffdesc_ring[jj].UserControl );
			TRACE(10, "%3u %2x 0x%08x", jj, 12
				, ((u32*)&(cfo_pci_sender[0].buffdesc_ring[jj]))[3] );
			TRACE(10, "%3u %2x 0x%08x", jj, 16
				, ((u32*)&(cfo_pci_sender[0].buffdesc_ring[jj]))[4] );
			TRACE(10, "%3u %2x 0x%016llx", jj, 20
				, cfo_pci_sender[0].buffdesc_ring[jj].SystemAddress );
			TRACE(10, "%3u %2x 0x%08x", jj, 28
				, cfo_pci_sender[0].buffdesc_ring[jj].NextDescPtr );
			TRACE(10, "%3u meta@%p[%d]=%u", jj
				, cfo_mmap_ptrs[0][S2C][CFO_MAP_META], jj
				, ((u32*)(cfo_mmap_ptrs[0][S2C][CFO_MAP_META]))[jj] );
			TRACE(10, "%3u 0x%08x 0x%08x 0x%08x 0x%08x", jj
				, ((u32*)&(cfo_pci_sender[0].databuffs[jj]))[0]
				, ((u32*)&(cfo_pci_sender[0].databuffs[jj]))[1]
				, ((u32*)&(cfo_pci_sender[0].databuffs[jj]))[2]
				, ((u32*)&(cfo_pci_sender[0].databuffs[jj]))[3]);
		}
		break;
	case M_IOC_BUF_XMIT:

		chn = arg >> 24;
		dir = S2C;

		// look at next descriptor and verify that it is complete
		// FIX ME --- race condition
		myIdx = cfo_channel_info_[chn][dir].swIdx;
		desc_S2C_p = idx2descVirtAdr(myIdx, chn, dir);
		if (desc_S2C_p->Complete != 1) {
			TRACE(11, "ioctl BUF_XMIT -EAGAIN myIdx=%u err=%d desc_S2C_p=%p 0x%016llx counts(in,sts)=%u,%u"
		        , myIdx, desc_S2C_p->Error, desc_S2C_p, *(u64*)desc_S2C_p, desc_S2C_p->ByteCount, desc_S2C_p->ByteCnt );
			return -EAGAIN;
		}
		TRACE(11, "cfo_ioctl: cmd=BUF_XMIT desc_S2C_p=%p 0x%016llx",desc_S2C_p, *(u64*)desc_S2C_p);

		desc_S2C_p->Complete = 0; // FIX ME --- race condition
		desc_S2C_p->ByteCount = arg & 0xfffff; // 20 bits max
		desc_S2C_p->ByteCnt = arg & 0xfffff; // 20 bits max
# if CFO_RECV_INTER_ENABLED
		desc_S2C_p->IrqComplete = 1;
		desc_S2C_p->IrqError = 1;
# else
		desc_S2C_p->IrqComplete = 0;
		desc_S2C_p->IrqError = 0;
# endif
		desc_S2C_p->StartOfPkt = 1;
		desc_S2C_p->EndOfPkt = 1;
		{	void * data = ((cfo_databuff_t*)(cfo_mmap_ptrs[chn][dir][CFO_MAP_BUFF]))[myIdx];
			TRACE(11, "ioctl BUF_XMIT myIdx=%u desc_S2C_p(%p)=%016llx ByteCnt=%d data(%p)[2-5]=%016llx %016llx %016llx %016llx"
			, myIdx, desc_S2C_p, *(u64*)desc_S2C_p, desc_S2C_p->ByteCnt, data, ((u64*)data)[2], ((u64*)data)[3], ((u64*)data)[4], ((u64*)data)[5] );
		}

		/* See Transmit (S2C) Descriptor Management
		   on page 56 of kc705_TRD_k7_pcie_dma_ddr3_base_Doc_13.4.pdf */
		nxtIdx = idx_add(myIdx, 1, chn, dir);
		descDmaAdr_swNxt = idx2descDmaAdr(nxtIdx, chn, dir);

		// update hwIdx here - MUST CHECK Buffer Descriptor Complete bit!!! (not register!!!)
		jj=cfo_channel_info_[chn][dir].num_buffs;
		hwIdx = cfo_channel_info_[chn][dir].hwIdx;
		while ( ((cfo_buffdesc_S2C_t*)idx2descVirtAdr(hwIdx,chn,dir))->Complete
		       && hwIdx != myIdx && jj--) {
			hwIdx = idx_add(hwIdx, 1, chn, dir);
			TRACE( 11, "ioctl BUF_XMIT cfo_channel_info_[chn][dir].hwIdx=%u swIdx=%u lps=%u"
			, hwIdx, cfo_channel_info_[chn][dir].swIdx, jj );
		}
		cfo_channel_info_[chn][dir].hwIdx = hwIdx;

		// Dma_mReadReg(cfo_pcie_bar_info.baseVAddr, 0x9108); // DEBUG read "user" reg.
		TRACE(11, "cfo_ioctl BUF_XMIT b4 WriteChnReg REG_SW_NEXT_BD(idx=%u) TELLING DMA TO GO (DO THIS BD) hwIdx=%u ->Complete=%d [0].Complete=%d"
		      ,nxtIdx, hwIdx, ((cfo_buffdesc_S2C_t*)idx2descVirtAdr(hwIdx,chn,dir))->Complete, ((cfo_buffdesc_S2C_t*)idx2descVirtAdr(0,chn,dir))->Complete );
		Dma_mWriteChnReg(chn, dir, REG_SW_NEXT_BD, descDmaAdr_swNxt);

		cfo_channel_info_[chn][dir].swIdx = nxtIdx;
		TRACE(11, "cfo_ioctl BUF_XMIT after WriteChnReg REG_SW_NEXT_BD swIdx=%u hwIdx=%u ->Complete=%d CmpltIdx=%u"
			, nxtIdx, hwIdx, ((cfo_buffdesc_S2C_t*)idx2descVirtAdr(hwIdx,chn,dir))->Complete
			, descDmaAdr2idx( Dma_mReadChnReg(chn,dir,REG_HW_NEXT_BD),chn,dir,0 ) );
		break;
	default:
		TRACE(10, "cfo_ioctl: unknown cmd");
		return (-1); // some error
	}
	TRACE(11, "cfo_ioctl: end");
	return (retval);
}   // cfo_ioctl

/////////////////////////////////////////////////////////////////////////////////////////////

static int ReadPCIState(struct pci_dev * pdev, m_ioc_pcistate_t * pcistate)
{
	int pos;
	u16 valw;
	u8  valb;
	unsigned long base;

	/* Since probe has succeeded, indicates that link is up. */
	pcistate->LinkState = LINK_UP;
	pcistate->VendorId = XILINX_VENDOR_ID;
	pcistate->DeviceId = XILINX_DEVICE_ID;

	/* Read Interrupt setting - Legacy or MSI/MSI-X */
	pci_read_config_byte(pdev, PCI_INTERRUPT_PIN, &valb);
	if (!valb)
	{
		if (pci_find_capability(pdev, PCI_CAP_ID_MSIX))
			pcistate->IntMode = INT_MSIX;
		else if (pci_find_capability(pdev, PCI_CAP_ID_MSI))
			pcistate->IntMode = INT_MSI;
		else
			pcistate->IntMode = INT_NONE;
	}
	else if ((valb >= 1) && (valb <= 4))
		pcistate->IntMode = INT_LEGACY;
	else
		pcistate->IntMode = INT_NONE;

	if ((pos = pci_find_capability(pdev, PCI_CAP_ID_EXP)))
	{
		/* Read Link Status */
		pci_read_config_word(pdev, pos + PCI_EXP_LNKSTA, &valw);
		pcistate->LinkSpeed = (valw & 0x0003);
		pcistate->LinkWidth = (valw & 0x03f0) >> 4;

		/* Read MPS & MRRS */
		pci_read_config_word(pdev, pos + PCI_EXP_DEVCTL, &valw);
		pcistate->MPS = 128 << ((valw & PCI_EXP_DEVCTL_PAYLOAD) >> 5);
		pcistate->MRRS = 128 << ((valw & PCI_EXP_DEVCTL_READRQ) >> 12);
	}
	else
	{
		printk("Cannot find PCI Express Capabilities\n");
		pcistate->LinkSpeed = pcistate->LinkWidth = 0;
		pcistate->MPS = pcistate->MRRS = 0;
	}

	/* Read Initial Flow Control Credits information */
	base = (unsigned long)(cfo_pcie_bar_info.baseVAddr);

	pcistate->InitFCCplD = XIo_In32(base + 0x901c) & 0x00000FFF;
	pcistate->InitFCCplH = XIo_In32(base + 0x9020) & 0x000000FF;
	pcistate->InitFCNPD = XIo_In32(base + 0x9024) & 0x00000FFF;
	pcistate->InitFCNPH = XIo_In32(base + 0x9028) & 0x000000FF;
	pcistate->InitFCPD = XIo_In32(base + 0x902c) & 0x00000FFF;
	pcistate->InitFCPH = XIo_In32(base + 0x9030) & 0x000000FF;
	pcistate->Version = XIo_In32(base + 0x9000);

	return 0;
}   // ReadPCIState





/////////////////////////////////////////////////////////////////////////////////////////////

void free_mem(void)
{
	unsigned       chn, jj, ii;

	// stop "app"
	Dma_mWriteReg(cfo_pcie_bar_info.baseVAddr
		, 0x9100, 0x80000000); // CFO reset, Clear Latched Errors
	msleep(10);

	for (chn = 0; chn < 2; ++chn)
	{
		// stop engines (both C2S and S2C channels)
		for (jj = 0; jj < 2; ++jj)  // this is "direction"
		{
			Dma_mWriteChnReg(chn, jj, REG_DMA_ENG_CTRL_STATUS
				, DMA_ENG_USER_RESET);
			msleep(10);
			Dma_mWriteChnReg(chn, jj, REG_DMA_ENG_CTRL_STATUS
				, DMA_ENG_RESET);
			msleep(10);
		}
	}

	for (chn = 0; chn < CFO_NUM_RECV_CHANNELS; ++chn)
	{
		for (ii = 0; ii < CFO_NUM_RECV_BUFFS; ++ii) {
			if (cfo_pci_recver[chn].databuffs[ii])
				dma_free_coherent(&cfo_pci_dev->dev
					, sizeof(cfo_databuff_t)
					, cfo_pci_recver[chn].databuffs[ii]
					, cfo_pci_recver[chn].databuffs_dma[ii]);
			if (cfo_pci_recver[chn].buffdesc_ring[ii])
				dma_free_coherent(&cfo_pci_dev->dev
					, sizeof(cfo_buffdesc_C2S_t)
					, cfo_pci_recver[chn].buffdesc_ring[ii]
					, cfo_pci_recver[chn].buffdesc_ring_dma[ii]);
		}
		kfree(cfo_pci_recver[chn].databuffs);
		kfree(cfo_pci_recver[chn].buffdesc_ring);
		kfree(cfo_pci_recver[chn].databuffs_dma);
		kfree(cfo_pci_recver[chn].buffdesc_ring_dma);
		free_pages((unsigned long)cfo_mmap_ptrs[chn][C2S][CFO_MAP_META], 0);
	}
	for (chn = 0; chn < CFO_NUM_SEND_CHANNELS; ++chn)
	{
		if (cfo_pci_sender[chn].databuffs)
			dma_free_coherent(&cfo_pci_dev->dev
				, sizeof(cfo_databuff_t)*CFO_NUM_SEND_BUFFS
				, cfo_pci_sender[chn].databuffs
				, cfo_pci_sender[chn].databuffs_dma);
		if (cfo_pci_sender[chn].buffdesc_ring)
			dma_free_coherent(&cfo_pci_dev->dev
				, sizeof(cfo_buffdesc_S2C_t)*CFO_NUM_SEND_BUFFS
				, cfo_pci_sender[chn].buffdesc_ring
				, cfo_pci_sender[chn].buffdesc_ring_dma);
		free_pages((unsigned long)cfo_mmap_ptrs[chn][S2C][CFO_MAP_META], 0);
	}
}   // free_mem


#define descIdx2dmaAddr( idx, ch, dir )


//////////////////////////////////////////////////////////////////////////////


static int __init init_cfo(void)
{
	int             ret = 0;          /* SUCCESS */
	unsigned        chn, ii, jj, dir;
	void           *va, *vb;
	unsigned long   databuff_sz;
	unsigned long   buffdesc_sz;
	u32             descDmaAdr;
	u32             ctrlStsVal;

	TRACE(0, "init_cfo");

	// fs interface, pci, memory, events(i.e polling)

	ret = cfo_fs_up();
	if (ret != 0) {
		ret = -2;
		goto out_fs;
	}
	ret = cfo_pci_up();
	if (ret != 0) {
		ret = -5;
		goto out_pci;
	}

	cfo_pci_dev = pci_get_device(XILINX_VENDOR_ID, XILINX_DEVICE_ID, NULL);
	if (cfo_pci_dev == NULL)
	{
		ret = -6;
		goto out_pci;
	}

	/* Use "Dma_" routines to init FPGA "user" application ("CFO") registers.
	   NOTE: a few more after dma engine setup (below).
	 */
	 //Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
	 //                , 0x9008, 0x00000002 ); // reset axi interface IP
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9100, 0x30000000); // Oscillator resets
	msleep(20);
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9100, 0x80000000); // CFO reset, Clear Oscillator resets
	msleep(20);
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9100, 0x00000000); // Clear CFO reset
	msleep(20);
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9118, 0x0000003f); // Reset all links
	msleep(20);
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9118, 0x00000000); // Clear Link Resets
  //Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
  //	              , 0x9114, 0x00003f3f ); // make sure all links are enabled

	TRACE(1, "init_cfo reset done bits: 0x%08x CFO_NUM_RECV_CHANNELS=%d CFO_NUM_RECV_BUFFS=%d CFO_NUM_SEND_BUFFS=%d"
	      , Dma_mReadReg((unsigned long)cfo_pcie_bar_info.baseVAddr,0x9138)
	      , CFO_NUM_RECV_CHANNELS, CFO_NUM_RECV_BUFFS, CFO_NUM_SEND_BUFFS );


	/* DMA Engine (channels) setup... (buffers and descriptors (and metadata)) */
	dir = C2S;
	for (chn = 0; chn < CFO_NUM_RECV_CHANNELS; ++chn)
	{
		TRACE(1, "init_cfo dma_alloc (#=%d)", CFO_NUM_RECV_BUFFS);

		va = kmalloc(CFO_NUM_RECV_BUFFS * sizeof(void *), GFP_KERNEL); // Array of data buffer pointers
		vb = kmalloc(CFO_NUM_RECV_BUFFS * sizeof(void *), GFP_KERNEL); // Array of buffdesc pointers
		if (va == NULL || vb == NULL) goto out;
		cfo_pci_recver[chn].databuffs = va;
		cfo_pci_recver[chn].buffdesc_ring = vb;

		va = kmalloc(CFO_NUM_RECV_BUFFS * sizeof(dma_addr_t), GFP_KERNEL); // dma addresses of data buffers
		vb = kmalloc(CFO_NUM_RECV_BUFFS * sizeof(dma_addr_t), GFP_KERNEL); // dma addresses of buffdesc memory
		if (va == NULL || vb == NULL) goto out;
		cfo_pci_recver[chn].databuffs_dma = va;
		cfo_pci_recver[chn].buffdesc_ring_dma = vb;

		for (ii = 0; ii < CFO_NUM_RECV_BUFFS; ++ii) {
			cfo_pci_recver[chn].databuffs[ii] =
				dma_alloc_coherent(&cfo_pci_dev->dev, sizeof(cfo_databuff_t), &(cfo_pci_recver[chn].databuffs_dma[ii]), GFP_KERNEL);
			cfo_pci_recver[chn].buffdesc_ring[ii] =
				dma_alloc_coherent(&cfo_pci_dev->dev, sizeof(cfo_buffdesc_C2S_t), &(cfo_pci_recver[chn].buffdesc_ring_dma[ii]), GFP_KERNEL);
			TRACE(1, "init_cfo cfo_pci_recver[%u].databuffs=%p databuffs_dma=0x%llx "
				"buffdesc_ring=%p buffdesc_ring_dma=0x%llx"
				, chn
				, cfo_pci_recver[chn].databuffs[ii]
				, cfo_pci_recver[chn].databuffs_dma[ii]
					, cfo_pci_recver[chn].buffdesc_ring[ii]
				, cfo_pci_recver[chn].buffdesc_ring_dma[ii]);
		}

		cfo_mmap_ptrs[chn][dir][CFO_MAP_BUFF] = cfo_pci_recver[chn].databuffs;
		//cfo_mmap_ptrs[chn][dir][CFO_MAP_META] = cfo_pci_recver[chn].buffdesc_ring;
		cfo_mmap_ptrs[chn][dir][CFO_MAP_META] = (void*)__get_free_pages(GFP_KERNEL, 0);

		TRACE(1, "init_cfo cfo_pci_recver[%u].meta@%p"
			, chn
			, cfo_mmap_ptrs[chn][dir][CFO_MAP_META]);

		cfo_channel_info_[chn][dir].chn = chn;
		cfo_channel_info_[chn][dir].dir = dir;
		cfo_channel_info_[chn][dir].buff_size = sizeof(cfo_databuff_t);
		cfo_channel_info_[chn][dir].num_buffs = CFO_NUM_RECV_BUFFS;

		for (jj = 0; jj < CFO_NUM_RECV_BUFFS; ++jj)
		{   /* ring -> link to next (and last to 1st via modulus) */
			cfo_pci_recver[chn].buffdesc_ring[jj]->NextDescPtr = cfo_pci_recver[chn].buffdesc_ring_dma[(jj + 1) % CFO_NUM_RECV_BUFFS];
			/* put the _buffer_ address in the descriptor */
			cfo_pci_recver[chn].buffdesc_ring[jj]->SystemAddress = cfo_pci_recver[chn].databuffs_dma[jj];
			/* and the size of the buffer also */
			cfo_pci_recver[chn].buffdesc_ring[jj]->RsvdByteCnt = sizeof(cfo_databuff_t);
#if CFO_RECV_INTER_ENABLED
			cfo_pci_recver[chn].buffdesc_ring[jj]->IrqComplete = 1;
			cfo_pci_recver[chn].buffdesc_ring[jj]->IrqError = 1;
#else
			cfo_pci_recver[chn].buffdesc_ring[jj]->IrqComplete = 0;
			cfo_pci_recver[chn].buffdesc_ring[jj]->IrqError = 0;
#endif
		}

		// now write to the HW...
		TRACE(1, "init_cfo write 0x%llx to 32bit reg", cfo_pci_recver[chn].buffdesc_ring_dma[0]);
		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_RESET);
		msleep(20);
		Dma_mWriteChnReg(chn, dir, REG_HW_NEXT_BD
			, (u32)cfo_pci_recver[chn].buffdesc_ring_dma[0]);
		cfo_channel_info_[chn][dir].hwIdx = 0;
		//TRACE( 1, "recver[chn=%d] REG_HW_NEXT_BD=%u"
		//    , chn, descDmaAdr2idx( (u32)cfo_pci_recver[chn].buffdesc_ring_dma,chn,dir));

		// set "DMA_ENG" (ie. HW) last/complete == SW NEXT to show "num avail" == 0
		descDmaAdr = idx2descDmaAdr(CFO_NUM_RECV_BUFFS - 1, chn, dir);
		Dma_mWriteChnReg(chn, dir, REG_SW_NEXT_BD, descDmaAdr);
		Dma_mWriteChnReg(chn, dir, REG_HW_CMPLT_BD, descDmaAdr);
		cfo_channel_info_[chn][dir].hwIdx = CFO_NUM_RECV_BUFFS - 1;
		cfo_channel_info_[chn][dir].swIdx = CFO_NUM_RECV_BUFFS - 1;

		ctrlStsVal = DMA_ENG_ENABLE;
#if CFO_RECV_INTER_ENABLED
		TRACE( 1, "init_cfo: ctrlStsVal |= DMA_ENG_INT_ENABLE" );
		ctrlStsVal |= DMA_ENG_INT_ENABLE;
#else
		TRACE( 1, "init_cfo: no DmaInterrrupt" );
#endif
		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, ctrlStsVal);

	}

	dir = S2C;
	for (chn = 0; chn < CFO_NUM_SEND_CHANNELS; ++chn)
	{
		databuff_sz = sizeof(cfo_databuff_t)*CFO_NUM_SEND_BUFFS;
		buffdesc_sz = sizeof(cfo_buffdesc_C2S_t)*CFO_NUM_SEND_BUFFS;
		TRACE(1, "init_cfo dma_alloc (#=%d) databuff_sz=%lu buffdesc_sz=%lu"
			, CFO_NUM_SEND_BUFFS, databuff_sz, buffdesc_sz);
		va = dma_alloc_coherent(&cfo_pci_dev->dev, databuff_sz
			, &cfo_pci_sender[chn].databuffs_dma
			, GFP_KERNEL);
		if (va == NULL) goto out;
		cfo_pci_sender[chn].databuffs = va;
		cfo_mmap_ptrs[chn][dir][CFO_MAP_BUFF] = va;
		va = dma_alloc_coherent(&cfo_pci_dev->dev, buffdesc_sz
			, &cfo_pci_sender[chn].buffdesc_ring_dma
			, GFP_KERNEL);
		if (va == NULL) goto out;
		cfo_pci_sender[chn].buffdesc_ring = va;
		cfo_mmap_ptrs[chn][dir][CFO_MAP_META] = (void*)__get_free_pages(GFP_KERNEL, 0);

		TRACE(1, "init_cfo cfo_pci_sender[%u].databuffs=%p databuffs_dma=0x%llx "
			"buffdesc_ring_dma=0x%llx meta@%p"
			, chn
			, cfo_pci_sender[chn].databuffs
			, cfo_pci_sender[chn].databuffs_dma
			, cfo_pci_sender[chn].buffdesc_ring_dma
			, cfo_mmap_ptrs[chn][dir][CFO_MAP_META]);
		cfo_channel_info_[chn][dir].chn = chn;
		cfo_channel_info_[chn][dir].dir = dir;
		cfo_channel_info_[chn][dir].buff_size = sizeof(cfo_databuff_t);
		cfo_channel_info_[chn][dir].num_buffs = CFO_NUM_SEND_BUFFS;
		for (jj = 0; jj < CFO_NUM_SEND_BUFFS; ++jj)
		{   /* ring -> link to next (and last to 1st via modulus) */
			cfo_pci_sender[chn].buffdesc_ring[jj].NextDescPtr =
				cfo_pci_sender[chn].buffdesc_ring_dma
				+ sizeof(cfo_buffdesc_S2C_t) * ((jj + 1) % CFO_NUM_SEND_BUFFS);
			/* put the _buffer_ address in the descriptor */
			cfo_pci_sender[chn].buffdesc_ring[jj].SystemAddress =
				cfo_pci_sender[chn].databuffs_dma
				+ sizeof(cfo_databuff_t) * jj;
			cfo_pci_sender[chn].buffdesc_ring[jj].Complete = 1; // Only reset just before giving to Engine -- enables check for complete -- which should be done before memcpy
		}

		// now write to the HW...
		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_RESET);
		msleep(20);
		// HW_NEXT and SW_Next registers to start of ring
		Dma_mWriteChnReg(chn, dir, REG_HW_NEXT_BD
			, (u32)cfo_pci_sender[chn].buffdesc_ring_dma);
		cfo_channel_info_[chn][dir].hwIdx = 0;
		Dma_mWriteChnReg(chn, dir, REG_SW_NEXT_BD
			, (u32)cfo_pci_sender[chn].buffdesc_ring_dma);
		cfo_channel_info_[chn][dir].swIdx = 0;

		// reset HW_Completed register
		Dma_mWriteChnReg(chn, dir, REG_HW_CMPLT_BD, 0);

		Dma_mWriteChnReg(chn, dir, REG_DMA_ENG_CTRL_STATUS, DMA_ENG_ENABLE);
	}

	/* Now, finish up with some more cfo fpga user application stuff... */
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9104, 0x80000040); // write max and min DMA xfer sizes
	Dma_mWriteReg((unsigned long)cfo_pcie_bar_info.baseVAddr
		, 0x9150, 0x00000010); // set ring packet size

	ret = cfo_event_up();

# if CFO_RECV_INTER_ENABLED
	/* Now enable interrupts using MSI mode */
	if (!pci_enable_msi(cfo_pci_dev))
	{
		TRACE( 1, "MSI enabled");
		MSIEnabled = 1;
	}

	ret = request_irq(cfo_pci_dev->irq, DmaInterrupt, IRQF_SHARED, "cfo", cfo_pci_dev);
	if (ret)
	{
		TRACE( 0, "xdma could not allocate interrupt %d", cfo_pci_dev->irq);
		TRACE( 0, "Unload driver and try running with polled mode instead");
	}
	Dma_mIntEnable((unsigned long)cfo_pcie_bar_info.baseVAddr);
# endif

	return (ret);

out:
	ret = -1;
	TRACE(0, "Error - freeing memory");
	free_mem();
out_pci:
	TRACE(0, "Error - destroying pci device");
	cfo_pci_down();
out_fs:
	TRACE(0, "Error - destroying filesystem entry");
	cfo_fs_down();
	return (ret);
}   // init_cfo


static void __exit exit_cfo(void)
{
	TRACE(1, "exit_cfo() called");


	Dma_mIntDisable((unsigned long)cfo_pcie_bar_info.baseVAddr);
	free_irq(cfo_pci_dev->irq, cfo_pci_dev);
	if (MSIEnabled) pci_disable_msi(cfo_pci_dev);

	// events, memory, pci, fs interface
	cfo_event_down();
	free_mem();
	cfo_pci_down();
	cfo_fs_down();
}   // exit_cfo


module_init(init_cfo);
module_exit(exit_cfo);

MODULE_AUTHOR("Ron Rechenmacher");
MODULE_DESCRIPTION("cfo pcie driver");
MODULE_LICENSE("GPL"); /* Get rid of taint message by declaring code as GPL */