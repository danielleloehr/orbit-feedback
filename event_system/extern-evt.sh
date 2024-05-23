# Setup external triggering on Input0 for local and backwards events
caput TEST-EVR:-In:0evr2:Trig:Ext-Sel Edge
caput TEST-EVR:-In:0evr2:Trig:Back-Sel Edge

# Send event code 0x10 intern and extern
caput TEST-EVR:-In:0evr2:Code:Back-SP 10
caput TEST-EVR:-In:0evr2:Code:Ext-SP 10
