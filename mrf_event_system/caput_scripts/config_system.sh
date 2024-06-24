#!/bin/bash
##
## Run this script to configure the event system and trigger events. This script 
## will configure one EVM and one EVR at a time. To configure a second EVR, you
## can re-run the script with a different EVR id.
##
## To configure a standalone EVR to send trigger events, please refer to extern_evt.sh
##

############################################
## Naming convention, defaults:
EVRSECTION="EVR1-1W1S14G"
EVMSECTION="EVM1-1W1S14G"
ES=":"
############################################

TRIGGER=0

enable_evr(){
	# Enable all EVRs
	caput $EVRSECTION:Link:Clk-SP 125		# IRQ+1
	caput $EVRSECTION:Ena-Sel "Enabled"
}

set_evm_clock(){
	# EVM intern clock selection
	caput $EVMSECTION:EvtClkSource-Sel "FracSyn (Int)"	# Upstream (fanout) is also fine
	caput $EVMSECTION:EvtClkFracSynFreq-SP 125.0
}


usage="$(basename "$0") [-h] [-evr <dev>] [-evm <dev>] [-trig]-- configure the timing system to trigger an interrupt event(0x10) on external input
where:
    -h  	show this help text
    --evr  	EVR device name (EVR_id and section name, default = EVR1-1W1S14G)
    --evm	EVM device name (EVM_id and section name, default = EVM1-1W1S14G)
    --trig	setup external trigger and send an event locally and in backwards mode (see extern_evt.sh)
"

VALID_ARGS=$(getopt -o :h --long evr:,evm:,trig -- "$@")
if [[ $? -ne 0 ]]; then
    exit 1;
fi

eval set -- "$VALID_ARGS"
while [ : ]; do
    case "$1" in
        -h) # display Help
            echo "$usage"
            echo
            exit;;
        --evr)
            EVRSECTION=$2
            echo "EVR device selection: $EVRSECTION"
            shift 2
            ;;
        --evm)
       	    EVMSECTION=$2
            echo "EVM device selection: $EVMSECTION"
            shift 2
            ;;	
        --trig)
       	    TRIGGER=1
       	    shift
       	    ;;
       	--) shift;
       	    break
       	    ;;
        \?) # incorrect option
            echo "Error: invalid option"
            echo "$usage"
            exit 1
            ;;
   esac
done

echo -e "Continuing with device selection: \n \t EVR=$EVRSECTION,\n \t EVM=$EVMSECTION"

set_evm_clock;
enable_evr;

## Set heartbeat signal if not disabled in source code
echo "Setting heartbeat signal"
./heartbeat.sh $EVMSECTION

if [[ $TRIGGER  -eq 1 ]]; then
	echo "Setting trigger event"
	./extern_evt.sh $EVRSECTION
fi

## Optional:
## EVR Pulse Generator configuration
#./pulser.sh $EVRSECTION

