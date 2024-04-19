#!../../bin/linux-x86_64/mrf
# Load this application bc it worked

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

## Setup EVR devices
mrmEvrSetupPCI("EVR1","07:00.0")
mrmEvrSetupPCI("EVR2","0d:00.0")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=mtca1c1s14gTest, ES=:evr1:, EVR=EVR1")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=mtca1c1s14gTest, ES=:evr2:, EVR=EVR2")

## Setup EVG
mrmEvgSetupPCI("EVG1", "05:00.0")		
dbLoadRecords("../../db/mtca-evm-300.db", "SYS=mtca1c1s14gTest, D=evg, EVG=EVG1")

iocInit()

## Select clock intern (125 MHz)
dbpf("mtca1c1s14gTest{evg-EvtClk}Source-Sel", "FracSyn (Int)")
dbpf("mtca1c1s14gTest{evg-EvtClk}FracSynFreq-SP", "125.0")

## Check if the event clock is correctly
#dbpr("mtca1c1s14gTest{evg-EvtClk}Frequency-RB")

## Enable EVRs
dbpf("mtca1c1s14gTest:evr1:Link:Clk-SP","125.0")
dbpf("mtca1c1s14gTest:evr1:Ena-Sel", "Enabled")

dbpf("mtca1c1s14gTest:evr2:Link:Clk-SP","125.0")
dbpf("mtca1c1s14gTest:evr2:Ena-Sel", "Enabled")

## Check status
dbpr("mtca1c1s14gTest:evr1:Link-Sts")
dbpr("mtca1c1s14gTest:evr2:Link-Sts")


## Take care of heartbeat, interrupts should disappear after this
dbpf("mtca1c1s14gTest{evg-Mxc:0}Prescaler-SP", "125000000")
dbpf("mtca1c1s14gTest{evg-Mxc:0}Frequency-RB", 1)
dbpf("mtca1c1s14gTest{evg-TrigEvt:0}EvtCode-SP", "122")
dbpf("mtca1c1s14gTest{evg-TrigEvt:0}TrigSrc-Sel", "Mxc0")


## Event Code 0x01 using MXC1 --> for 600 Hz signal
dbpf("mtca1c1s14gTest{evg-Mxc:1}Prescaler-SP", "208333")
dbpf("mtca1c1s14gTest{evg-Mxc:1}Frequency-RB", 1)
dbpf("mtca1c1s14gTest{evg-TrigEvt:1}EvtCode-SP", "1")
dbpf("mtca1c1s14gTest{evg-TrigEvt:1}TrigSrc-Sel", "Mxc1")

##dbpf("mtca1c1s14gTest:evr1:Evt:Blink0-SP", "1")

## Event code 0x7D (Concentrator interrupt event) using MXC2 --> 150 Hz
dbpf("mtca1c1s14gTest{evg-Mxc:2}Prescaler-SP", "833333")
dbpf("mtca1c1s14gTest{evg-Mxc:2}Frequency-RB", 1)
dbpf("mtca1c1s14gTest{evg-TrigEvt:2}EvtCode-SP", "125")
dbpf("mtca1c1s14gTest{evg-TrigEvt:2}TrigSrc-Sel", "Mxc2")
## You should be able to observe interrupts in /proc/interrupts now


## Pulse settings (Output)
## EVR1 --> 600 Hz ADC event
dbpf("mtca1c1s14gTest-DlyGen:0:evr1:Ena-Sel", "1")
dbpf("mtca1c1s14gTest-DlyGen:0:evr1:Evt:Trig0-SP", "1")
dbpf("mtca1c1s14gTest-DlyGen:0:evr1:Width-SP" ,"1")
dbpf("mtca1c1s14gTest-Out:FP0:evr1:Src:Pulse-SP", "0")

## EVR2 --> 150 Hz  
dbpf("mtca1c1s14gTest-DlyGen:0:evr2:Ena-Sel", "1")
dbpf("mtca1c1s14gTest-DlyGen:0:evr2:Evt:Trig0-SP", "125")
dbpf("mtca1c1s14gTest-DlyGen:0:evr2:Width-SP" ,"1")
dbpf("mtca1c1s14gTest-Out:FP0:evr2:Src:Pulse-SP", "0")



