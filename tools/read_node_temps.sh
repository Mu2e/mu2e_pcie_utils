#!/bin/bash

# Configuration
CORE_TEMP_THRESHOLD=60
VERBOSE_SET=${VERBOSE+1}
VERBOSE=${VERBOSE:-0}
HOSTID=`hostname|sed 's/^mu2e//'|sed 's/[.].*$//'`
  
# Default to verbose if run interactively
if [ -z "$VERBOSE_SET" ] && [ $VERBOSE -eq 0 ] && [ -t 0 ];then
  VERBOSE=1
fi

# From NOvA's Loadshed script
#MAIL_RECIPIENTS_ERROR="eflumerf@fnal.gov,mu2e_tdaq_developers@fnal.gov" # comma-separated list
MAIL_RECIPIENTS_ERROR="eflumerf@fnal.gov,rrivera@fnal.gov" # comma-separated list
function send_error_mail()
{
        echo "$2"| mail -s "`date`: $1" $MAIL_RECIPIENTS_ERROR
}

core0temp=`sensors|grep "Package id 0:"|grep -Eo "[0-9][0-9]"|head -1`
core1temp=`sensors|grep "Package id 1:"|grep -Eo "[0-9][0-9]"|head -1`

if [ $VERBOSE -ge 1 ];then
  echo "CPU 0 temp is $core0temp, CPU 1 temp is $core1temp"
fi

if [ $core0temp -gt $CORE_TEMP_THRESHOLD ] || [ $core1temp -gt $CORE_TEMP_THRESHOLD ]; then
  send_error_mail "Node temperature error" "One or more CPU temperatures are above threshold! $HOSTID: $core0temp $core1temp"
fi
