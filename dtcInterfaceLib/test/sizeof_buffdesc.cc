// This file (tt.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb  7, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: .emacs.gnu,v $
// rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

#include <stdint.h>
#include <stdio.h>  // printf

// typedef unsigned u64;
#include "mu2e_driver/xdma_hw.h"

int main(int, char *[])
{
	printf("sizeof(mu2e_buffdesc_S2C_t)=%lu (u32)=%lu (u64)=%lu\n", sizeof(mu2e_buffdesc_S2C_t), sizeof(u32),
		   sizeof(u64));
	printf(
		"UserControl   offset: %lu\n"
		"CardAddress   offset: %lu\n"
		"SystemAddress offset: %lu\n"
		"NextDescPtr   offset: %lu\n",
		(unsigned long)&((mu2e_buffdesc_S2C_t *)0)->UserControl, (unsigned long)&((mu2e_buffdesc_S2C_t *)0)->CardAddress,
		(unsigned long)&((mu2e_buffdesc_S2C_t *)0)->SystemAddress,
		(unsigned long)&((mu2e_buffdesc_S2C_t *)0)->NextDescPtr);
	return (0);
}  // main
