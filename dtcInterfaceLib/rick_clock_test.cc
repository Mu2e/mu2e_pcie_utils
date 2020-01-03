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
   usage: %s <start|stop|dump>\n\
          %s read <offset>\n\
          %s write <offset> <val>\n\
          %s write_loopback_data [-p<packet_cnt>] [data]...\n\
examples: %s start\n\
",            \
		basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0]), basename(argv[0])

void write(unsigned long addr, unsigned long val, int fd)
{
	m_ioc_reg_access_t reg_access;
	reg_access.reg_offset = addr;
	reg_access.access_type = 1;
	reg_access.val = val;
	int sts = ioctl(fd, M_IOC_REG_ACCESS, &reg_access);
	if (sts)
	{
		perror("ioctl M_IOC_REG_ACCESS write");
		exit(1);
	}
}

void write_and_check(unsigned long addr, int fd)
{
	m_ioc_reg_access_t reg_access;
	reg_access.reg_offset = addr;
	reg_access.access_type = 1;
	reg_access.val = 1;
	int sts = ioctl(fd, M_IOC_REG_ACCESS, &reg_access);
	if (sts)
	{
		perror("ioctl M_IOC_REG_ACCESS write");
		exit(1);
	}

	while (reg_access.val == 1)
	{
		reg_access.access_type = 0;
		sts = ioctl(fd, M_IOC_REG_ACCESS, &reg_access);
		if (sts)
		{
			perror("ioctl M_IOC_REG_ACCESS read");
			exit(1);
		}
		usleep(100);
	}
}

int main(int argc, char* argv[])
{
	int sts = 0;
	int fd;
	mu2edev dev;
	char devfile[11];
	int dtc = -1;

	int opt_v = 0;
	int opt;
	while (1)
	{
		int option_index = 0;
		static struct option long_options[] = {
			/* name,   has_arg, int* flag, val_for_flag */
			{"mem-to-malloc", 1, 0, 'm'},
			{0, 0, 0, 0},
		};
		opt = getopt_long(argc, argv, "?hm:Vv", long_options, &option_index);
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
			case 'd':
				dtc = strtol(optarg, NULL, 0);
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", opt);
		}
	}

	if (dtc == -1)
	{
		char* dtcE = getenv("DTCLIB_DTC");
		if (dtcE != NULL)
			dtc = strtol(dtcE, NULL, 0);
		else
			dtc = 0;
	}

	snprintf(devfile, 11, "/dev/" MU2E_DEV_FILE, dtc);

	fd = open(devfile, O_RDONLY);
	if (fd == -1)
	{
		perror("open");
		return (1);
	}

	// set xtal on timing card to 200mhz
	// freeze DCO
	write(0x9168, 0x5d891000, fd);
	write_and_check(0x916c, fd);

	// write new values for FRREQ, HS_DIV, N1
	write(0x9168, 0x5d076000, fd);
	write_and_check(0x916c, fd);

	write(0x9168, 0x5d08c300, fd);
	write_and_check(0x916c, fd);

	write(0x9168, 0x5d091000, fd);
	write_and_check(0x916c, fd);

	write(0x9168, 0x5d0a6900, fd);
	write_and_check(0x916c, fd);

	write(0x9168, 0x5d0b8d00, fd);
	write_and_check(0x916c, fd);

	write(0x9168, 0x5d0c7700, fd);
	write_and_check(0x916c, fd);

	// unfreeze DCO
	write(0x9168, 0x5d890000, fd);
	write_and_check(0x916c, fd);

	// Set NewFreq bit
	write(0x9168, 0x5d874000, fd);
	write_and_check(0x916c, fd);

	printf("sts=%d\n", sts);
	return (0);
}  // main
