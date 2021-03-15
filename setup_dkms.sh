#!/bin/bash

# This script prepares the repo to serve as a DKMS build area (in /usr/src/pcie_linux_kernel_module-VERSION)
# Any code changes to the TRACE, mu2e, or pci_devel_main modules will need to have a either a new version,
# or a dkms -m pcie_linux_kernel_module -v VERSION uninstall, remove, add, build, install cycle

# To install (as root):
# cd /usr/src
# git clone https://cdcvs.fnal.gov/projects/pcie_linux_kernel_module pcie_linux_kernel_module-version
# echo mu2e >/etc/modules-load.d/mu2e.conf
# dkms add -m pcie_linux_kernel_module -v VERSION
# dkms autoinstall

if ! [ -d trace-git ]; then
  git clone https://cdcvs.fnal.gov/projects/trace-git
fi

cat >Makefile <<EOF

EXTRA_SYMBOLS=KBUILD_EXTRA_SYMBOLS=$PWD/trace-git/src_module/Module.symvers
TRACE_INC=$PWD/trace-git/include

all: 
	make -C trace-git/src_module/
	make -C mu2e_driver/ CC=/usr/bin/gcc EXTRA_SYMBOLS=\${EXTRA_SYMBOLS} TRACE_INC=\${TRACE_INC}
	make -C mcs/basic_driver/ CC=/usr/bin/gcc EXTRA_SYMBOLS=\${EXTRA_SYMBOLS} TRACE_INC=\${TRACE_INC}

clean:
	make -C trace-git/src_module/ clean
	make -C mu2e_driver/ clean
	make -C mcs/basic_driver/ clean

EOF
