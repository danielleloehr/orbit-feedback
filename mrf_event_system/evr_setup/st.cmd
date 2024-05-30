#!../../bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

#######################################################
# Please configure these accordingly
epicsEnvSet("EVR_PCI", "07:00.0")
epicsEnvSet("EVR_SECTION", "EVR1-1W1S14G")  
epicsEnvSet("ES", ":")       
epicsEnvSet("FREQ", "125")
epicsEnvSet("dsh", ":")
#######################################################

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

## Setup EVR
mrmEvrSetupPCI("EVR1","$(EVR1_PCI)")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=$(EVR_SECTION), dsh=$(dsh), ES=$(ES), EVR=EVR1")

## To setup a second EVR
#epicsEnvSet("EVR2_PCI", "05:00.0")
#epicsEnvSet("EVM_PCI", "0d:00.0")
#mrmEvrSetupPCI("EVR2","$(EVR2_PCI)")
#dbLoadRecords("../../db/evr-mtca-300u.db", "P=$(P)-EVR:, ES=evr2:, EVR=EVR2")

## To setup an EVM 
#mrmEvgSetupPCI("EVM", "$(EVM_PCI)")		
#dbLoadRecords("../../db/evm-mtca-300.uv.db","P=$(P)-EVM:,EVG=EVM,FRF=$(FREQ),FEVT=$(FREQ)")
#dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-U:,EVG=EVM,T=U,FRF=$(FREQ),FEVT=$(FREQ)")
#dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-D:,EVG=EVM,T=D,FRF=$(FREQ),FEVT=$(FREQ)")

iocInit()

##########################################################
