// This file (mcs.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// May 19, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
const char *rev = "$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <getopt.h>  // getopt_long
#include <libgen.h>  // basename
#include <stdio.h>   // printf
#include <stdlib.h>  // setenv

#include "trace.h"  // TRACE

#include "pcidevl_ioctl.h"

#define USAGE \
	"\
usage: %s [opts] <mcs_file>\n\
    -e            just erase (then exit)\n\
   --swap-data    swap the data\n\
",            \
		basename(argv[0])

int g_fd;

// 32bit byte and half word swap
inline uint32_t swab32(uint32_t a)
{
	return ((0xFF000000 & a) >> 24) | ((0x00FF0000 & a) >> 8) | ((0x0000FF00 & a) << 8) | ((0x000000FF & a) << 24);
}  // swab32

int set_prom_program_data_reg32(uint32_t data)
{
	ioc_ioop ioop;
	ioop.offset = 0x9400;
	ioop.ops_mask = ioop_write;
	ioop.write_val = data;
	TRACE(5, "set_prom_program_status_reg32 0x%08x ==>", ioop.write_val);
	int sts = ioctl(g_fd, IOC_IOOP, &ioop);
	return sts;
}

int get_prom_program_status_reg32(uint32_t *val)
{
	ioc_ioop ioop;
	ioop.offset = 0x9404;
	ioop.ops_mask = ioop_read;
	int sts = ioctl(g_fd, IOC_IOOP, &ioop);
	*val = ioop.read_val;
	TRACE(5, "get_prom_program_status_reg32 val=0x%08x", ioop.read_val);
	return sts;
}

int main(int argc, char *argv[])
{
	int opt_v = 0;
	int opt_just_erase = 0;
	int opt_data_swap = 0;
	extern int optind; /* for getopt */
	char devfile[13];  /* /dev/pcidevX */

	while (1)
	{
		int opt, option_index = 0;
		static struct option long_options[] = {
			/* name,   has_arg, int* flag, val_for_flag */
			{"swap-data", 0, 0, 1},
			{0, 0, 0, 0},
		};
		opt = getopt_long(argc, argv, "?hVve", long_options, &option_index);
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
			case 'e':
				opt_just_erase = 1;
				break;
			case 1:
				opt_data_swap = 1;
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", opt);
		}
	}
	if (argc - optind < 2)
	{
		printf("Need mcs_file and device number\n");
		printf(USAGE);
		return (-1);
	}

	setenv("TRACE_LVLS", "0xf", 0);  // setenv( "TRACE_NAME", "mcs", 0 );
	TRACE_CNTL("lvlset", 0xffffffffULL, strtoull(getenv("TRACE_LVLS"), NULL, 0), 0ULL);
	TRACE_CNTL("mode", 3ULL);
	TRACE(1, "hello");

	snprintf(devfile, 13, "/dev/" PCIDEVL_DEV_FILE, atoi(argv[optind++]));

	int fd = open(devfile, O_RDONLY);
	if (fd == -1)
	{
		perror("open"); /*return (1);*/
	}
	g_fd = fd;

	FILE *stream = fopen(argv[optind], "r");

	int lines = 0, data_bytes = 0;
	unsigned StartBlockSet = 0, StartBlock, EndBlock, LastAddr = 0;
	int bytecount;
	unsigned address, RecType, data, checksum;
	int sts;
	while ((sts = fscanf(stream, ":%2x%4x%2x", (unsigned *)&bytecount, &address, &RecType)) == 3)
	{
		if (RecType == 4)
		{
			if (StartBlockSet != 1)
			{
				StartBlockSet = 1;
				sts = fscanf(stream, "%4x", &StartBlock);
				if (sts != 1)
				{
					TRACE(0, "Error getting StartBlock");
					return -1;
				}
			}
			else
			{
				if (fscanf(stream, "%4x", &EndBlock) != 1)
				{
					TRACE(0, "Error getting EndBlock");
					return -1;
				}
			}
			bytecount -= 2;
		}
		else if (RecType == 0)
		{
			LastAddr = address;
		}
		while (bytecount--)
		{
			sts = fscanf(stream, "%2x", &data);
			if (sts != 1)
			{
				printf("did not get data\n");
				return -1;
			}
		}
		sts = fscanf(stream, "%2x\n", &checksum);
		lines++;
	}
	int FileSize = (((EndBlock << 16) | LastAddr) / 2) - ((StartBlock << 16) / 2);  // file size in bytes
	TRACE(1, "got %d lines. StartBlock=0x%x EndBlock=0x%x LastAddr=0x%x FileSize=%d", lines, StartBlock, EndBlock,
		  LastAddr, FileSize);

	// rewind
	fseek(stream, 0, SEEK_SET);
	lines = 0;

	uint32_t ReadMask = 0x1;
	uint32_t ReadData;
	sts = get_prom_program_status_reg32(&ReadData);
	if (sts) TRACE(0, "unexpected status");
	if ((ReadData & ReadMask) != ReadMask)
	{
		TRACE(0, "not ready");
		return -1;
	}

	// unlock all blocks
	uint32_t StartBlock_w = StartBlock / 2;  // bytes to "words" (1 word == 2 bytes)

	sts = set_prom_program_data_reg32(0x53410000 | (StartBlock_w << 8));
	if (sts) TRACE(0, "unexpected status");
	sts = set_prom_program_data_reg32(0x45000000 | (((EndBlock << 16) | LastAddr) / 2));
	if (sts) TRACE(0, "unexpected status");
	sts = set_prom_program_data_reg32(0x556e6c6b);
	if (sts) TRACE(0, "unexpected status");

	ReadData = 0;
	uint32_t checks = 0;
	while ((ReadData & ReadMask) != ReadMask)
	{
		sts = get_prom_program_status_reg32(&ReadData);
		if (sts) TRACE(0, "unexpected status");
		checks++;
	}
	TRACE(1, "after unlock, checks=%u ReadData=0x%x", checks, ReadData);

	// Erase blocks
	sts = set_prom_program_data_reg32(0x45726173);
	if (sts) TRACE(0, "unexpected status");
	ReadData = checks = 0;
	while ((ReadData & ReadMask) != ReadMask)
	{
		sts = get_prom_program_status_reg32(&ReadData);
		if (sts) TRACE(0, "unexpected status");
		checks++;
	}
	TRACE(1, "after Erase, checks=%u ReadData=0x%x", checks, ReadData);

	if (opt_just_erase) return (0);

	// Program
	sts = set_prom_program_data_reg32(0x50726f67);
	if (sts) TRACE(0, "unexpected status");

	ReadMask = 0xff0000;

	while ((sts = fscanf(stream, ":%2x%4x%2x", (unsigned *)&bytecount, &address, &RecType)) == 3)
	{
		switch (RecType)
		{
			case 0:
				TRACE(4, "Data Record - bytecount=0x%02x, address0x%04x", bytecount, address);
				ReadData = checks = 0;
				if ((data_bytes % 64) == 0)
					while ((ReadData & ReadMask) != 0x800000)
					{
						sts = get_prom_program_status_reg32(&ReadData);
						if (sts) TRACE(0, "unexpected status");
						if ((ReadData & ReadMask) == 0x900000)
						{
							TRACE(0, "program error ReadData=0x%x checks=%u data_bytes=%d", ReadData, checks, data_bytes);
							return -1;
						}
						checks++;
						if (checks == 10000000)
						{
							TRACE(0, "program timeout ReadData=0x%x", ReadData);
							return -1;
						}
					}
				TRACE(1, "before set_prom_program_data_reg32, checks=%u data_bytes=%d", checks, data_bytes);
				while (bytecount > 0)
				{
					sts = fscanf(stream, "%8x", &data);
					if (sts != 1)
					{
						printf("did not get data\n");
						return -1;
					}

					if (opt_data_swap)
						sts = set_prom_program_data_reg32(swab32(data));
					else
						sts = set_prom_program_data_reg32(data);
					if (sts) TRACE(0, "unexpected status");
					bytecount -= 4;
					data_bytes += 4;
				}
				sts = fscanf(stream, "%2x\n", &checksum);
				if (sts != 1)
				{
					printf("did not get checksum\n");
					return -1;
				}
				break;
			case 1:
				TRACE(3, "End of File - bytecount=0x%02x, address0x%04x", bytecount, address);
				while (bytecount--)
				{
					sts = fscanf(stream, "%2x", &data);
					if (sts != 1)
					{
						printf("did not get data\n");
						return -1;
					}
				}
				sts = fscanf(stream, "%2x\n", &checksum);
				if (sts != 1)
				{
					printf("did not get checksum\n");
					return -1;
				}
				break;
			case 4:
				TRACE(3, "Extended Linear Address - bytecount=0x%02x, address0x%04x", bytecount, address);
				while (bytecount--)
				{
					sts = fscanf(stream, "%2x", &data);
					if (sts != 1)
					{
						printf("did not get data\n");
						return -1;
					}
				}
				sts = fscanf(stream, "%2x\n", &checksum);
				if (sts != 1)
				{
					printf("did not get checksum\n");
					return -1;
				}
				break;
			default:
				TRACE(1, "Programming - ignoring record type %d\n", RecType);
				while (bytecount--)
				{
					sts = fscanf(stream, "%2x", &data);
					if (sts != 1)
					{
						printf("did not get data\n");
						return -1;
					}
				}
				sts = fscanf(stream, "%2x\n", &checksum);
				if (sts != 1)
				{
					printf("did not get checksum\n");
					return -1;
				}
		}
		lines++;
	}
	TRACE(1, "got %d lines, %d data_bytes", lines, data_bytes);

	return (0);
}  // main
