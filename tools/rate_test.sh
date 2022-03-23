#!/bin/bash

function do_rate_test() {

source /mu2e/ups/setup
source /cvmfs/mu2e.opensciencegrid.org/artexternals/setups

setup pcie_linux_kernel_module v2_05_11 -qe20:s112:prof
source Setup_DTC.sh

# Configure DTC Library
DTCLIB_SIM_ENABLE=N
DTCLIB_SIM_FILE=/home/eflumerf/Desktop/mu2e-product-build/DTC_packets.bin
DTCLIB_DTC=${1:-0}
testname=${2:-test}
cpulist=${3:-0-99}

DTCRegDump -d$DTCLIB_DTC|head

# Load sim file into DTC
DTC_Reset $DTCLIB_DTC
#mu2eUtil --dtc $DTCLIB_DTC reset_detemu
mu2eUtil --dtc $DTCLIB_DTC -G -p buffer_test

# Read sim file from DTC
taskset -c $cpulist mu2eUtil --dtc $DTCLIB_DTC -G -n 1000 -q 2  buffer_test >${testname}_DTC${DTCLIB_DTC}_CPU${cpulist}_${HOSTNAME}.log

}

# DTC 0 CPU 0, DTC 1 CPU 1
do_rate_test 0 test00_11 2-5 &
do_rate_test 1 test00_11 14-17 &
wait

# DTC 0 CPU 0, DTC 1 CPU 0
do_rate_test 0 test00_10 2-5 &
do_rate_test 0 test00_10 6-9 &
wait

# DTC 0 CPU 1, DTC 1 CPU 0
do_rate_test 0 test01_10 18-21 &
do_rate_test 0 test01_10  6-9 &
wait

# DTC 0 CPU 1, DTC 1 CPU 1
do_rate_test 0 test01_11 18-21 &
do_rate_test 1 test01_11 14-17 &
wait

# DTC 0 AUTO, DTC 1 AUTO
do_rate_test 0 test0A_1A &
do_rate_test 1 test0A_1A &
wait
