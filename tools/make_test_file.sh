#!/bin/bash

# DTC Config
export DTCLIB_SIM_ENABLE=E
export DTCLIB_NUM_TRACKER_BLOCKS=212
export DTCLIB_NUM_CALORIMETER_BLOCKS=60
export DTCLIB_NUM_CALORIMETER_HITS=10
export DTCLIB_NUM_CRV_BLOCKS=0

# Create file
mu2eUtil -Q -n 1000 --binary-file-mode DTC_packets.bin verify_stream