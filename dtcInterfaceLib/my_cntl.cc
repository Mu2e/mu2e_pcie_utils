// This file (my_cntl.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb  6, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
static char* rev = (char*)"$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <fcntl.h>   // open, O_RDONLY
#include <getopt.h>  // getopt_long
#include <libgen.h>  // basename
#include <stdio.h>   // printf
#include <stdlib.h>  // strtoul
#include <string.h>  // strcmp

#include "mu2e_driver/mu2e_mmap_ioctl.h"  // m_ioc_cmd_t
#include "mu2edev.h"                      // mu2edev

#define USAGE \
	"\
   usage: %s <start|stop|dump|spy>\n\
          %s read <offset>\n\
          %s write <offset> <val>\n\
          %s write_loopback_data [-p<packet_cnt>] [data]...\n\
		  %s spy [optsmask]  # see code for optsmask\n\
examples: %s start\n\
options:\n\
 -d<dtvdevnum>     \n\
 -l<loops>\n\
 -c<channel>    0=ROC data, 1=DCS\n\
 -t     test/check   - currently for read\n\
",            \
		basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0])

int main(int argc, char* argv[])
{
	int sts = 0;
	int fd = -1;
	char* cmd;
	m_ioc_cmd_t ioc_cmd;
	m_ioc_reg_access_t reg_access;
	mu2edev dev;
	char devfile[11];
	int dtc = -1;

	int opt_v = 0, opt_chn=0;
	int opt;
	//int opt_packets = 8;
	unsigned opt_loops = 1;  // saying -l10 will cause the thing to happen 10 times (i.e "loop back around 9 times")
	unsigned opt_test_check = 0;
	while (1)
	{
		int option_index = 0;
		static struct option long_options[] = {
			/* name,   has_arg, int* flag, val_for_flag */
			{"mem-to-malloc", 1, 0, 'm'},
			{0, 0, 0, 0},
		};
		opt = getopt_long(argc, argv, "?hm:Vvp:c:d:l:t", long_options, &option_index);
		if (opt == -1) break;
		switch (opt)
		{
			case '?':
			case 'h':
				printf(USAGE);
				exit(0);
				break;
			case 'V':
				printf("%s\n", rev);
				return (0);
				break;
			case 'v':
				opt_v++;
				break;
			case 'p':
				//opt_packets = strtoul(optarg, NULL, 0);
				break;
			case 'd':
				dtc = strtol(optarg, NULL, 0);
				break;
			case 'c':
				opt_chn = strtol(optarg, NULL, 0);
				break;
			case 'l':
				opt_loops = strtol(optarg, NULL, 0);
				break;
			case 't':
				opt_test_check = 1;
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", opt);
		}
	}
	if (argc - optind < 1)
	{
		printf("Need cmd\n");
		printf(USAGE);
		exit(0);
	}
	cmd = argv[optind++];
	TRACE(TLVL_TRACE, "cmd=%s", cmd);
	//	TRACE(2,"opt_packets=%i", opt_packets);

	if (dtc == -1)
	{
		char* dtcE = getenv("DTCLIB_DTC");
		if (dtcE != NULL)
			dtc = strtol(dtcE, NULL, 0);
		else
			dtc = 0;
	}


	if (strcmp(cmd, "start") == 0 || 
		strcmp(cmd, "stop") == 0 || 
		strcmp(cmd, "dump") == 0 || 
		strcmp(cmd, "read") == 0 )
	{
		snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, dtc);

		fd = open(devfile, O_RDONLY);
		if (fd == -1)
		{
			perror("open");
			return (1);
		}
	}
	else if(strcmp(cmd, "write") == 0 ||
			strcmp(cmd, "spy") == 0 )
	{
		dev.init(DTCLib::DTC_SimMode_Disabled, dtc);
	}
	else
	{
		printf("unknown cmd %s\n", cmd);
		return (1);
	}

	if (strcmp(cmd, "start") == 0)
	{
		sts = ioctl(fd, M_IOC_TEST_START, &ioc_cmd);
	}
	else if (strcmp(cmd, "stop") == 0)
	{
		sts = ioctl(fd, M_IOC_TEST_STOP, &ioc_cmd);
	}
	else if (strcmp(cmd, "read") == 0)
	{
		if (argc < 3)
		{
			printf(USAGE);
			return (1);
		}
		reg_access.reg_offset = strtoul(argv[optind], NULL, 0);
		reg_access.access_type = 0;
		int need_initial = 1;
		unsigned prv_val;
		int val_changed = 0;
		for (unsigned ll = 0; ll < opt_loops; ++ll)
		{
			sts = ioctl(fd, M_IOC_REG_ACCESS, &reg_access);
			if (sts)
			{
				perror("ioctl M_IOC_REG_ACCESS read");
				return (1);
			}
			if (opt_test_check)
			{
				if (need_initial)
				{
					need_initial = 0;
					prv_val = reg_access.val;
					printf("initial val: 0x%08x\n", reg_access.val);
				}
				else if (reg_access.val != prv_val)
				{
					printf("val changed from 0x%08x - new val: 0x%08x\n", prv_val, reg_access.val);
					prv_val = reg_access.val;
					val_changed++;
				}
			}
			else
				printf("0x%08x\n", reg_access.val);
		}
		if (val_changed)
			printf("value changed %d times\n", val_changed);
		else if (opt_test_check)
			printf("value did not change during %u loops\n", opt_loops);
	}
	else if (strcmp(cmd, "write") == 0)
	{
		if (argc < 4)
		{
			printf(USAGE);
			return (1);
		}
	
		uint32_t writeAddress = strtoul(argv[optind++], NULL, 0);
		uint32_t writeValue = strtoul(argv[optind++], NULL, 0);
		uint32_t readbackValue;
		dev.write_register_checked(writeAddress,100,writeValue,
			&readbackValue);
		if(writeValue != readbackValue)	
			printf("NOTE: Write value mismatch 0x%X vs readback 0x%X\n", writeValue,readbackValue);	

		// reg_access.reg_offset = strtoul(argv[optind++], NULL, 0);
		// reg_access.access_type = 1;
		// reg_access.val = strtoul(argv[optind++], NULL, 0);
		// sts = ioctl(fd, M_IOC_REG_ACCESS, &reg_access);
		// if (sts)
		// {
		// 	perror("ioctl M_IOC_REG_ACCESS write");
		// 	return (1);
		// }
	}
	else if (strcmp(cmd, "dump") == 0)
	{
		sts = ioctl(fd, M_IOC_DUMP); // dumps to kernel TRACE (/proc/trace/buffer or printk (NAME=mu2e_main))
		if (sts)
		{
			perror("ioctl M_IOC_REG_ACCESS write");
			return (1);
		}
	}
	else if (strcmp(cmd, "spy") == 0)
	{
		unsigned optsmsk=0;
		if (argc - optind >= 1) optsmsk = strtoul(argv[optind], NULL, 0);
		std::cout << "Calling dev.spy(chn) with optsmsk="<<optsmsk<<"\n";
		dev.spy(opt_chn, optsmsk);
	}
	// else if (strcmp(cmd,"write_loopback_data") == 0)
	//   {
	// 	  sts = dev.init();
	// 	  printf("1dev.devfd_=%d\n", dev.get_devfd_() );
	// 	  if (sts) { perror("dev.init"); return (1); }

	// 	  int regs[]={0x9108,0x9168};  // loopback enable
	// 	  for (unsigned ii=0; ii<(sizeof(regs)/sizeof(regs[0])); ++ii)
	// 		{
	// 		  reg_access.reg_offset  = regs[ii];
	// 		  reg_access.access_type = 1;//wr
	// 		  reg_access.val         = 0x1;
	// 		  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
	// 		  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }
	// 		}
	// 	  reg_access.reg_offset  = 0x9114;  // link enable register
	// 	  reg_access.access_type = 1;//wr
	// 	  reg_access.val         = 0x101; // Link 0 - both xmit and recv
	// 	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
	// 	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }

	// 	  reg_access.reg_offset  = 0x2004;  // DMA channel-0 C2S control reg
	// 	  reg_access.access_type = 1;//wr
	// 	  reg_access.val         = 0x100; // bit 8 is DMA Enable (offset 0x2*** is for C2S chan 0)
	// 	  sts = ioctl( fd, M_IOC_REG_ACCESS, &reg_access );
	// 	  if (sts) { perror("ioctl M_IOC_REG_ACCESS write"); return (1); }

	// 	  int chn=0;
	// 	  struct
	// 	  {   DataHeaderPacket hdr;
	// 	    DataPacket       data[8];
	// 	  } data={{{0}}};
	// 	  printf("sizeof(hdr)=%lu\n", sizeof(data.hdr) );
	// 	  printf("sizeof(data)=%lu\n", sizeof(data.hdr) );
	// 	  data.hdr.s.TransferByteCount = 64;
	// 	  if (argc > 2) data.hdr.s.TransferByteCount = strtoul( argv[optind++],NULL,0 );
	// 	  data.hdr.s.Valid = 1;
	// 	  data.hdr.s.PacketType = 5;  // could be overwritten below
	// 	  data.hdr.s.LinkID = 0;
	// 	  data.hdr.s.PacketCount = (data.hdr.s.TransferByteCount-16)/16;// minus header, / packet size
	// 	  //data.hdr.s.ts10 = 0x3210;
	// 	  data.hdr.s.ts32 = 0x7654;
	// 	  data.hdr.s.ts54 = 0xba98;
	// 	  data.hdr.s.data32 = 0xdead;
	// 	  data.hdr.s.data54 = 0xbeef;
	// 	  unsigned pcnt=opt_packets;
	// 	  if (pcnt > (sizeof(data.data)/sizeof(data.data[0])))
	// 		{   pcnt = (sizeof(data.data)/sizeof(data.data[0]));
	// 		}
	// 	  for (unsigned jj=0; jj<pcnt; ++jj)
	// 		{
	// 		  data.data[jj].data10 = (jj<<8)|1;
	// 		  data.data[jj].data32 = (jj<<8)|2;
	// 		  data.data[jj].data54 = (jj<<8)|3;
	// 		  data.data[jj].data76 = (jj<<8)|4;
	// 		  data.data[jj].data98 = (jj<<8)|5;
	// 		  data.data[jj].dataBA = (jj<<8)|6;
	// 		  data.data[jj].dataDC = (jj<<8)|7;
	// 		  data.data[jj].dataFE = (jj<<8)|8;
	// 		}
	// 	  uint16_t *data_pointer = (uint16_t*)&data;
	// 	  unsigned wrds=0;
	// 	  for (int kk=optind; kk<argc; ++kk, ++wrds)
	// 		{   printf("argv[kk]=%s\n",argv[kk] );
	// 		  if (wrds > (sizeof(data.data)/sizeof(uint16_t))) goto out;
	// 		  *data_pointer++ = (uint16_t)strtoul(argv[kk],NULL,0);
	// 		}
	// 	out:

	// 	  printf("2dev.devfd_=%d\n", dev.get_devfd_() );
	// 	  sts = dev.write_loopback_data( chn, &data,data.hdr.s.TransferByteCount );
	//   }
	else
	{
		printf("unknown cmd %s\n", cmd);
		return (1);
	}

	printf("sts=%d\n", sts);
	return (0);
}  // main
