#!../../bin/linux-x86_64/mrf
# Load this application bc it worked

#- You may have to change mytest to something else
#- everywhere it appears in this file

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

mrmEvrSetupPCI("EVR1","07:00.0")

dbLoadRecords("../../db/evr-mtca-300u.db", "P=TEST, ES=evr:, EVR=EVR1")

## EVR Pulser Alias

mrmEvgSetupPCI("EVG1", "d:0.0")

dbLoadRecords("../../db/mtca-evm-300.db", "SYS=TEST, D=evg, EVG=EVG1")

iocInit()

#mrmEvgSoftTime("EVG1")

dbpf("TEST{evg-EvtClk}Source-Sel", "FracSyn (Int)")
dbpf("TEST{evg-EvtClk}FracSynFreq-SP", "125.0")
dbpf("TESTevr:Link:Clk-SP","125.0")
dbpf("TESTevr:Ena-Sel", "Enabled")

## Check status
dbpr(TESTevr:Link-Sts)

## Take care of heartbeat
dbpf("TEST{evg-Mxc:0}Prescaler-SP", "125000000")
dbpf("TEST{evg-Mxc:0}Frequency-RB", 1)
dbpf("TEST{evg-TrigEvt:0}EvtCode-SP", "122")
dbpf("TEST{evg-TrigEvt:0}TrigSrc-Sel", "Mxc0")

## Interrupt issuing event at 1 Hz
dbpf("TEST{evg-Mxc:1}Prescaler-SP", "125000000")
dbpf("TEST{evg-Mxc:1}Frequency-RB", 1)

dbpf("TEST{evg-TrigEvt:1}EvtCode-SP", "125")
dbpf("TEST{evg-TrigEvt:1}TrigSrc-Sel", "Mxc1")
dbpf("TESTevr:Evt:Blink0-SP", "125")

## Output
dbpf("TEST-DlyGen:0evr:Ena-Sel", "1")
dbpf("TEST-DlyGen:0evr:Evt:Trig0-SP", "125")
dbpf("TEST-DlyGen:0evr:Width-SP" ,"1")
dbpf("TEST-Out:FP0evr:Src:Pulse-SP", "0")

