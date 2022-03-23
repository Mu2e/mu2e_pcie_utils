#!/bin/bash

source /mu2e/ups/setup
source /cvmfs/mu2e.opensciencegrid.org/artexternals/setups

setup pcie_linux_kernel_module v2_05_14 -qe20:s114:prof # Update as needed

# DTC Config
export DTCLIB_SIM_ENABLE=E
export DTCLIB_NUM_TRACKER_BLOCKS=212
export DTCLIB_NUM_CALORIMETER_BLOCKS=60
export DTCLIB_NUM_CALORIMETER_HITS=10
export DTCLIB_NUM_CRV_BLOCKS=0

# Create file
mu2eUtil -H -n 1000 --binary-file-mode DTC_packets.bin