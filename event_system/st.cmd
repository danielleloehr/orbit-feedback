#!../../bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("EVR1_PCI", "05:00.0")
epicsEnvSet("EVR2_PCI", "07:00.0")
epicsEnvSet("EVM_PCI", "0d:00.0")
epicsEnvSet("P", "TEST")
epicsEnvSet("FREQ", "125")

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

## Setup EVR
mrmEvrSetupPCI("EVR1","$(EVR1_PCI)")
mrmEvrSetupPCI("EVR2","$(EVR2_PCI)")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=$(P)-EVR:, ES=evr1:, EVR=EVR1")
dbLoadRecords("../../db/evr-mtca-300u.db", "P=$(P)-EVR:, ES=evr2:, EVR=EVR2")
##dbLoadRecords("../../db/evr-mtca-300u.uv.db","P=$(P)-EVR1:, EVR=EVR1,PNDELAY=PNDELAY,PNWIDTH=PNWIDTH,FRF=$(FREQ),FEVT=$(FREQ)")

## Setup EVM
mrmEvgSetupPCI("EVM", "$(EVM_PCI)")		

dbLoadRecords("../../db/evm-mtca-300.uv.db","P=$(P)-EVM:,EVG=EVM,FRF=$(FREQ),FEVT=$(FREQ)")
dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-U:,EVG=EVM,T=U,FRF=$(FREQ),FEVT=$(FREQ)")
dbLoadRecords("../../db/evm-mtca-300-evr.uv.db","P=$(P)-D:,EVG=EVM,T=D,FRF=$(FREQ),FEVT=$(FREQ)")

iocInit()

##########################################################
