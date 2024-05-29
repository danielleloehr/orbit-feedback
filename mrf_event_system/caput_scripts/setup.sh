##
## Run this script if an EVM and an EVR are in the same system!!!
##
## To configure the standalone EVR, please refer to extern_evt.sh
##

## Naming convention -- todo: pass as arguments
EVRSECTION="EVR1-1W1S14G"
EVMSECTION="EVM1-1W1S14G"
ES=":"
############################################

# Enable all EVRs
caput $EVRSECTION:Link:Clk-SP 125		# IRQ+1
caput $EVRSECTION:Link:Ena-Sel "Enabled"

# EVM intern clock selection
caput $EVMSECTION:EvtClkSource-Sel "FracSyn (Int)"	# Upstream (fanout) is also fine
caput $EVMSECTION:EvtClkFracSynFreq-SP 125.0

# EVR Pulse Generator configuration
##./pulser.sh

## Set heartbeat signal if not disabled in source code
./heartbeat.sh EVMSECTION
