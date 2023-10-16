#!/bin/bash

# Configuration
PCIE_VERSION=${PCIE_VERSION:-v2_08_00}
PCIE_QUALS=${PCIE_QUALS:-e20:s118:prof}
EPICS_VERSION=${EPICS_VERSION:-v7_0_6_1}
EPICS_QUALS=${EPICS_QUALS:-e20}
DTC_TEMP_THRESHOLD=${DTC_TEMP_THRESHOLD:-75}
DTC_MAX_TEMP=255
FIREFLY_TEMP_THRESHOLD=${FIREFLY_TEMP_THRESHOLD:-60}
FIREFLY_MAX_TEMP=120
VERBOSE_SET=${VERBOSE+1}
VERBOSE=${VERBOSE:-0}
SHUTDOWN_ON_ERROR=${SHUTDOWN_ON_ERROR:-1}
HOSTID=`hostname|sed 's/^mu2e-//'|sed 's/[.].*$//'`
export EPICS_CA_AUTO_ADDR_LIST=NO
export EPICS_CA_ADDR_LIST=
export EPICS_CA_NAME_SERVERS=mu2e-dcs-01
  
DO_CA=`ls ~mu2edcs/.caput_enabled 2>/dev/null|wc -l`
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )


# Default to verbose if run interactively
if [ -z "$VERBOSE_SET" ] && [ $VERBOSE -eq 0 ] && [ -t 0 ];then
  VERBOSE=1
  echo "Shutdown on error: $SHUTDOWN_ON_ERROR, DO_CA: $DO_CA"
fi

# Check for updates
pushd $SCRIPT_DIR
if [ -d .git ]; then
  git fetch
  git diff --quiet --exit-code "origin/develop" "tools/read_dtc_temps.sh"
  if [ $? -eq 1 ];then
    if [[ $VERBOSE -ne 0 ]]; then
      echo "read_dtc_temps.sh updated, relaunching!"
    fi
    git pull
    popd
    exec $0
    exit 1
  fi
  if [[ $VERBOSE -ne 0 ]]; then
    echo "read_dtc_temps.sh is latest version"
  fi
fi
popd

# From NOvA's Loadshed script
#MAIL_RECIPIENTS_ERROR="eflumerf@fnal.gov,mu2e_tdaq_developers@fnal.gov" # comma-separated list
MAIL_RECIPIENTS_ERROR="eflumerf@fnal.gov,rrivera@fnal.gov,grakness@fnal.gov,gahs@phys.ksu.edu,sws-admin@fnal.gov" # comma-separated list
function send_error_mail()
{
        echo "$2"| mail -s "`date`: $1" $MAIL_RECIPIENTS_ERROR
}

nfsready=0
while [ $nfsready -eq 0 ];do
  /bin/mountpoint -q /home >/dev/null 2>&1
  ret=$?
  if [ $ret -eq 0 ];then
    nfsready=1
  else
    sleep 60
  fi
done

source /mu2e/ups/setup
setup epics $EPICS_VERSION -q$EPICS_QUALS
setup mu2e_pcie_utils $PCIE_VERSION -q$PCIE_QUALS

errstring=
warnstring=

for ii in {0..3}; do

  if [ -e /dev/mu2e$ii ]; then
    
    if [[ $VERBOSE -ne 0 ]]; then
      echo "Reading temperatures for $HOSTNAME DTC $ii"
    fi

    dtctemp=`my_cntl -d$ii read 0x9010|grep 0x`
    tempvalue=`echo "print int(round(($dtctemp * 503.975 / 4096) - 273.15))"|python -`

    if [[ $VERBOSE -ne 0 ]]; then
      echo "DTC Temperature: $tempvalue"
    fi
    if [[ $tempvalue -gt $DTC_TEMP_THRESHOLD ]] && [[ $tempvalue -le $DTC_MAX_TEMP ]]; then
       errstring="${errstring+$errstring$'\n'}DTC Overtemp $HOSTNAME:/dev/mu2e$ii: $tempvalue!"
    fi
    if [[ ${#EPICS_BASE} -ne 0 ]] && [ $DO_CA -eq 1 ]; then
	caput Mu2e:CompStatus:$HOSTID:DTC$ii:dtctemp $tempvalue >/dev/null 2>&1
        if [ $? -ne 0 ];then
          warnstring="${warnstring+$warnstring$'\n'}Error pushing PV to EPICS: Mu2e:CompStatus:$HOSTID:DTC$ii:dtctemp $tempvalue"
        fi
    fi

    # RX Firefly
    #enable IIC on Firefly
    my_cntl -d$ii write 0x93a0 0x00000200 >/dev/null 2>&1
    #Device address, register address, null, null
    my_cntl -d$ii write 0x9298 0x54160000 >/dev/null 2>&1
    #read enable
    my_cntl -d$ii write 0x929c 0x00000002 >/dev/null 2>&1
    #disable IIC on Firefly
    my_cntl -d$ii write 0x93a0 0x00000000 >/dev/null 2>&1
    #read data: Device address, register address, null, temp in 2's compl.
    rxtempval=`my_cntl -d$ii read 0x9298|grep 0x`
    tempvalue=`echo "print int($rxtempval & 0xFF)"|python -`

    if [[ $VERBOSE -ne 0 ]]; then
      echo "RX Firefly temperature: $tempvalue"
    fi
    if [[ $tempvalue -gt $FIREFLY_TEMP_THRESHOLD ]] && [[ $tempvalue -le $FIREFLY_MAX_TEMP ]]; then
      errstring="${errstring+$errstring$'\n'}RX Firefly Overtemp $HOSTNAME:/dev/mu2e$ii: $tempvalue!"
    fi
    if [[ ${#EPICS_BASE} -ne 0 ]] && [ $DO_CA -eq 1 ]; then
	caput Mu2e:CompStatus:$HOSTID:DTC$ii:rxtemp $tempvalue >/dev/null 2>&1
        if [ $? -ne 0 ];then
          warnstring="${warnstring+$warnstring$'\n'}Error pushing PV to EPICS: Mu2e:CompStatus:$HOSTID:DTC$ii:rxtemp $tempvalue"
        fi
    fi

    # TX Firefly
    my_cntl -d$ii write 0x93a0 0x00000100 >/dev/null 2>&1
    my_cntl -d$ii write 0x9288 0x50160000 >/dev/null 2>&1
    my_cntl -d$ii write 0x928c 0x00000002 >/dev/null 2>&1
    my_cntl -d$ii write 0x93a0 0x00000000 >/dev/null 2>&1
    txtempval=`my_cntl -d$ii read 0x9288|grep 0x`
    tempvalue=`echo "print int($txtempval & 0xFF)"|python -`

    if [[ $VERBOSE -ne 0 ]]; then
      echo "TX Firefly temperature: $tempvalue"
    fi
    if [[ $tempvalue -gt $FIREFLY_TEMP_THRESHOLD ]] && [[ $tempvalue -le $FIREFLY_MAX_TEMP ]]; then
      errstring="${errstring+$errstring$'\n'}TX Firefly Overtemp $HOSTNAME:/dev/mu2e$ii: $tempvalue!"
    fi
    if [[ ${#EPICS_BASE} -ne 0 ]] && [ $DO_CA -eq 1 ]; then
	caput Mu2e:CompStatus:$HOSTID:DTC$ii:txtemp $tempvalue >/dev/null 2>&1
        if [ $? -ne 0 ];then
          warnstring="${warnstring+$warnstring$'\n'}Error pushing PV to EPICS: Mu2e:CompStatus:$HOSTID:DTC$ii:txtemp $tempvalue"
        fi
    fi
    
    # TX/RX Firefly
    my_cntl -d$ii write 0x93a0 0x00000400 >/dev/null 2>&1
    my_cntl -d$ii write 0x92a8 0x50160000 >/dev/null 2>&1
    my_cntl -d$ii write 0x92ac 0x00000002 >/dev/null 2>&1
    my_cntl -d$ii write 0x93a0 0x00000000 >/dev/null 2>&1
    txrxtempval=`my_cntl -d$ii read 0x92a8|grep 0x`
    tempvalue=`echo "print int($txrxtempval & 0xFF)"|python -`

    if [[ $VERBOSE -ne 0 ]]; then
      echo "TX/RX Firefly temperature: $tempvalue"
    fi
    if [[ $tempvalue -gt $FIREFLY_TEMP_THRESHOLD ]] && [[ $tempvalue -le $FIREFLY_MAX_TEMP ]]; then
      errstring="${errstring+$errstring$'\n'}TX/RX Firefly Overtemp $HOSTNAME:/dev/mu2e$ii: $tempvalue!"
    fi
    if [[ ${#EPICS_BASE} -ne 0 ]] && [ $DO_CA -eq 1 ]; then
	caput Mu2e:CompStatus:$HOSTID:DTC$ii:rxtxtemp $tempvalue >/dev/null 2>&1
        if [ $? -ne 0 ];then
          warnstring="${warnstring+$warnstring$'\n'}Error pushing PV to EPICS: Mu2e:CompStatus:$HOSTID:DTC$ii:rxtxtemp $tempvalue"
        fi
    fi

  fi

done
    
if [ ${#errstring} -gt 0 ]; then
  errstring="${errstring+$errstring$'\n'}Shutdown on Error mode: $SHUTDOWN_ON_ERROR"
  send_error_mail "Temperature Errors on $HOSTNAME" "$errstring"
  if [ $VERBOSE -ne 0 ];then
    echo "Temperature Errors on $HOSTNAME: $errstring" >&2
  fi
  if [[ $SHUTDOWN_ON_ERROR -ne 0 ]]; then
    /sbin/shutdown -h +1 # Make sure to delay so that email goes out
  fi
fi

if [ ${#warnstring} -gt 0 ]; then
#  send_error_mail "EPICS Errors on $HOSTNAME" "$warnstring"

  if [ $VERBOSE -ne 0 ];then
    echo "EPICS Errors on $HOSTNAME: $warnstring" >&2
  fi
fi

