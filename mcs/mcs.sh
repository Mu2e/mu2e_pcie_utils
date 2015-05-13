#! /bin/sh
 # This file (mcs.sh) was created by Ron Rechenmacher <ron@fnal.gov> on
 # Jan  3, 2015. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: .emacs.gnu,v $
 # rev='$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $'

test $# -eq 1 || { echo "usage: `basename $0` <file>"; exit; }

file=$1; shift

# look for pci_devel_main.ko
if   [ -f "${CETPKG_BUILD-}"/linux_driver/bin/pci_devel_main.ko ];then
   DEVMOD="${CETPKG_BUILD-}"/linux_driver/bin/pci_devel_main.ko
elif [ -f "${PCIE_LINUX_KERNEL_DRIVER_FQ_DIR-}"/bin/pci_devel_main.ko ];then
   DEVMOD="${PCIE_LINUX_KERNEL_DRIVER_FQ_DIR-}"/bin/pci_devel_main.ko
else
   echo "ERROR - can't find pci_devel_main.ko"
   exit
fi

# find the mcs and devl executables
if type mcs >/dev/null;then :;else
    echo "ERROR - mcs and devl executables not found - setup pcie_linux_kernel_driver"
    exit
fi


pids=`lsof -t /dev/mu2e 2>/dev/null` && kill $pids

lsmod | grep pci_devel_main >/dev/null && rmmod pci_devel_main
sleep 1
lsmod | grep mu2e           >/dev/null && rmmod mu2e
lsmod | grep TRACE          >/dev/null && rmmod TRACE

# (re)setup TRACE (no get functions defined in this script
. /mu2e/ups/setup
test -n "${SETUP_TRACE-}"\
 && { xx=$SETUP_TRACE; unsetup TRACE; eval setup $xx;}\
 || setup TRACE

insmod $TRACE_DIR/module/`uname -r`/TRACE.ko
echo 1 >|/sys/module/TRACE/parameters/trace_allow_printk
export TRACE_FILE=/proc/trace/buffer
tonSg 0-7; tonMg 0-15


insmod $DEVMOD

devl uint32 0x9000

export TRACE_NAME=mcs
mcs $file | tee /tmp/mcs.out\
 |{ xx=11;while xx=`expr $xx - 1`;do IFS= read ln;echo "$ln";done;echo ...;tail;}

echo "Now Power Cycle!!!!!!!!!"