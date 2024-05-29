#!../../bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("EVR1_PCI", "07:00.0")
epicsEnvSet("EVM_PCI", "0d:00.0")
epicsEnvSet("EVR_SECTION", "EVR1-1W1S14G")  
epicsEnvSet("EVM_SECTION", "EVM1-1W1S14G")  
epicsEnvSet("ES", ":")        
epicsEnvSet("FREQ", "125")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

## Setup EVR
mrmEvrSetupPCI("EVR1","$(EVR1_PCI)")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=$(EVR_SECTION):, ES=$(ES), EVR=EVR1")

## Setup EVM
mrmEvgSetupPCI("EVM", "$(EVM_PCI)")		
dbLoadRecords("../../db/evm-mtca-300.uv.db","P=$(EVM_SECTION),EVG=EVM,FRF=$(FREQ),FEVT=$(FREQ)")
##dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-U:,EVG=EVM,T=U,FRF=$(FREQ),FEVT=$(FREQ)")
##dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-D:,EVG=EVM,T=D,FRF=$(FREQ),FEVT=$(FREQ)")

iocInit()

##########################################################
