## Configure a standalone EVR to work as an EVM
## Send an external trigger event upstream

#EVRSECTION="EVR1-1W1S14G"	## 1W1S7G
#or
EVRSECTION=$1

# Setup external triggering on Input0 for local events
# Send event code 0x10 intern
caput $EVRSECTION:In:0:Trig:Ext-Sel Edge
caput $EVRSECTION:In:0:Code:Ext-SP 10

# Setup for backwards events
caput $EVRSECTION:In:0:Trig:Back-Sel Edge
caput $EVRSECTION:In:0:Code:Back-SP 10
