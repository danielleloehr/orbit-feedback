#!bin/linux-x86_64/mrf

< envPaths

## Register all support components
dbLoadDatabase("dbd/mrf.dbd")
mrf_registerRecordDeviceDriver(pdbbase)

#######################################################
# Please configure these accordingly
epicsEnvSet("EVM_PCI", "0d:00.0")
epicsEnvSet("EVM_SECTION", "EVM1-1W1S14G")  
epicsEnvSet("ES", ":")       
epicsEnvSet("FREQ", "125")
#######################################################

epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","1000000")

## To setup an EVM 
mrmEvgSetupPCI("EVM", "$(EVM_PCI)")		
dbLoadRecords("db/evm-mtca-300.uv.db","P=$(EVM_SECTION):,EVG=EVM,FRF=$(FREQ),FEVT=$(FREQ)")
#dbLoadRecords("db/evm-mtca-300-evr.uv.db","P=$(P)-U:,EVG=EVM,T=U,FRF=$(FREQ),FEVT=$(FREQ)")
#dbLoadRecords("db/evm-mtca-300-evr.uv.db","P=$(P)-D:,EVG=EVM,T=D,FRF=$(FREQ),FEVT=$(FREQ)")

iocInit()

##########################################################
