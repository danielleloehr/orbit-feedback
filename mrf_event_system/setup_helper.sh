##
## Startup script helper to run st.cmd from current directory
## Select device: EVR / EVM
##

setup_device(){
    # Path to setup_helper
    CURR_PATH=`pwd`
    # EPICS mrfioc2 path
    MRFIOC2_PATH=~/Documents/
    #~/epics/base/modules/mrfioc2/

    cd $MRFIOC2_PATH

    case $DEVICE in 
        "evr")
            $CURR_PATH/evr_setup/st.cmd
            ;;
        "evm")
            $CURR_PATH/evm_setup/st.cmd
            ;;
        "all")
            $CURR_PATH/st.cmd
            ;;
        *)
            echo $DEVICE: not a valid device
            exit 1
            ;;   
    esac
}

usage="$(basename "$0") [-h] [-d <dev>] -- Run EPICS startup script of MRF device
where:
    -h  show this help text
    -d  select device (evr, evm)
"

while getopts ":hd:" option; do
    case $option in
        h) # display Help
            echo "$usage"
            echo
            exit;;
        d)  # setup device
            DEVICE+=("$OPTARG")
            echo "Device selection: $DEVICE"
            setup_device
            ;;
        :)  printf "missing argument for -%s\n" "$OPTARG"
            echo "$usage"
            exit 1
            ;;
        \?) # incorrect option
            echo "Error: invalid option"
            echo "$usage"
            exit 1
            ;;
    esac
done

