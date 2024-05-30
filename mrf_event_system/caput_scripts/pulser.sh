## Pulse generation settings for testing

#EVRSECTION="EVR1-1W1S14G"
#or
EVRSECTION=$1

# EVR1: Event 10 --> TTL0
echo "Set EVR1 OUT0 TTL to Event Code 0x10"

caput $EVRSECTION:DlyGen:0:Ena-Sel 1
caput $EVRSECTION:DlyGen:0:Evt:Trig0-SP 10
caput $EVRSECTION:DlyGen:0:Width-SP 1
caput $EVRSECTION:Out:FP0:Src:Pulse-SP 0

#EVR2: Event 10 --> TTL0
#echo "Set EVR2 OUT0 TTL to Event Code 0x10"

#caput TEST-EVR:-DlyGen:0evr2:Ena-Sel 1
#caput TEST-EVR:-DlyGen:0evr2:Evt:Trig0-SP 10
#caput TEST-EVR:-DlyGen:0evr2:Width-SP 1
#caput TEST-EVR:-Out:FP0evr2:Src:Pulse-SP 0
