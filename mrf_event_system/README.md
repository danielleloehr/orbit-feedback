Please edit the following arguments accordingly:

If both EVR and EVM must run in the same crate:
- EVR name(=EVR_SECTION) and PCI address(=EVR1_PCI) in *st.cmd* (working directory)
- Check the identifier in *mrmEvrSetupPCI("EVR1","$(EVR1_PCI)")*
- EVM name(=EVM_SECTION) and PCI address(=EVM_PCI) in *st.cmd* (working directory)
- Check the identifier in *mrmEvgSetupPCI("EVM", "$(EVM_PCI)")*

To configure a standalone EVR:
- EVR name and PCI address in *evr_setup/st.cmd*
- For clarity, use the identifier **EVR2** in in *mrmEvgSetupPCI()* if the standalone EVR is the second EVR in the crate

To configure a standalone EVM:
- EVM name and PCI address in *evm_setup/st.cmd*

**Please note all EVM_SECTION and EVR_SECTION names. These will be used in caput_scripts/config_system.sh**