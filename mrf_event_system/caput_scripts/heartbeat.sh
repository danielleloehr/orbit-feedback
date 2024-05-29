## Use the same naming convention
EVMSECTION=$1

## Take care of heartbeat monitor
## Event code 0x7A triggered on Prescaler Mxc0
caput $EVMSECTION:Mxc0Prescaler-SP 125000000
caput $EVMSECTION:TrigEvt0EvtCode-SP 122
caput $EVMSECTION:TrigEvt0TrigSrc-Sel "Mxc0"
