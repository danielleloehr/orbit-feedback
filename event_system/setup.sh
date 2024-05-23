# Enable all EVRs
caput TEST-EVR:evr1:Link:Clk-SP 125		# IRQ+1
caput TEST-EVR:evr1:Ena-Sel "Enabled"
caput TEST-EVR:evr2:Link:Clk-SP 125		# IRQ+1
caput TEST-EVR:evr2:Ena-Sel "Enabled"

# EVM intern clock selection
caput TEST-EVM:EvtClkSource-Sel "FracSyn (Int)"	# Upstream (fanout) is also fine
caput TEST-EVM:EvtClkFracSynFreq-SP 125.0

# EVR Pulse Generator configuration
./pulser.sh

## Set heartbeat signal if not disabled in source code
## ./heartbeat.sh
