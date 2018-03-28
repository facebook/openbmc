#!/bin/bash

ME_UTIL="/usr/local/bin/me-util"
CMD_DIR="/usr/local/fbpackages/crashdump"
SENSOR_HISTORY=180
SLOT=$1

# read command from stdin
function execute_cmd {
  # if ME_UTIL cannot be executed, return
  [ -x $ME_UTIL ] || \
    return

  # Adapt command to ME-UTIL format
  cat | \
    sed 's/^0x/0xb8 0x40 0x57 0x01 0x00 0x/g' $1 | \
      $ME_UTIL $SLOT --file /dev/stdin | \
        sed 's/^57 01 00 //g'
}

# ARG: Bus[8], Device[5], Function[3], Register[12]
function PCI_Config_Addr {
  RESULT=0
  # BUS
  RESULT=$((RESULT | (($1 & 0xFF) << 20) ))
  # DEV
  RESULT=$((RESULT | (($2 & 0x1F) << 15) ))
  # FUN
  RESULT=$((RESULT | (($3 & 0x7) << 12) ))
  # REG
  RESULT=$((RESULT | (($4 & 0xFFF) << 0) ))
  printf "0x%x 0x%x 0x%x 0x%x"\
    $(( RESULT & 0xFF )) \
    $(( (RESULT >> 8) & 0xFF )) \
    $(( (RESULT >> 16) & 0xFF )) \
    $(( (RESULT >> 24) & 0xFF ))
  return 0
}

function find_end_device {
  echo "Find Root Port Bus $1 Dev $2 Fun $3..."
  RES=$(echo "0x30 0x06 0x05 0x61 0x00 $(PCI_Config_Addr $1 $2 $3 0x18)" | execute_cmd)
  echo "$RES"
  CC=$(echo $RES| awk '{print $1;}')
  SECONDARY_BUS=0x$(echo $RES| awk '{print $3;}')
  SUBORDINATE_BUS=0x$(echo $RES| awk '{print $4;}')
  if [ "$CC" == "40" ] && [ "$SECONDARY_BUS" != "0x00" ] && [ "$SUBORDINATE_BUS" != "0x00" ]; then
    for (( BUS=$SECONDARY_BUS; BUS<=$SUBORDINATE_BUS; BUS++ )); do
      BUS_MS_NIBBLE=$(printf "%x" $((BUS >> 4)) )
      BUS_LS_NIBBLE=$(printf "%x" $((BUS & 0xF)) )
      [ -r $CMD_DIR/crashdump_pcie_bus ] && \
        sed "s/<BUSM>/$BUS_MS_NIBBLE/g; s/<BUSL>/$BUS_LS_NIBBLE/g" $CMD_DIR/crashdump_pcie_bus | execute_cmd
    done
  fi
}

function pcie_dump {
  # CPU and PCH
  [ -r $CMD_DIR/crashdump_pcie ] && \
    cat $CMD_DIR/crashdump_pcie | execute_cmd

  # Buses
  RES=$(echo "0x30 0x06 0x05 0x61 0x00 0xcc 0x20 0x04 0x00" | execute_cmd)
  echo "Get CPUBUSNO: $RES"

  # Completion Code
  CC=$(echo $RES| awk '{print $1;}')
  
  # Success
  if [ "$CC" == "40" ]; then
    # CPU root port - Bus 0x16/0x64, Dev 0~3, Fun:0
    # Root port buses
    ROOT_BUSES=$(echo $RES | awk '{printf "0x%s 0x%s", $3, $4;}')
    ROOT_BUS=$(echo $RES | awk '{printf "0x%s", $2;}')
    echo ROOT_BUSES=$ROOT_BUSES
    for ROOT in $ROOT_BUSES; do
      for DEV in {0..3}; do
        find_end_device $ROOT $DEV 0
      done
    done

    # CPU root port - Bus 0,Dev 0, Fun:0
    echo ROOT_BUS=$ROOT_BUS
    find_end_device $ROOT_BUS 0 0

    # PCH root port - Bus 0,Dev 0x1B, Fun:0
    find_end_device $ROOT_BUS 0x1B 0

    # PCH root port - Bus 0,Dev 0x1D, Fun:0
    find_end_device $ROOT_BUS 0x1D 0

    # PCH root port - Bus 0,Dev 0x1D, Fun:4
    find_end_device $ROOT_BUS 0x1D 4
  fi

  # CPU root port - Bus 0x00/0x16/0x64, Dev 0~3, Fun:0
}

function dwr_dump {

  echo
  echo DWR assert check:
  echo =================
  echo

  # Get biosscratchpad7[26]: DWR assert check
  RES=$(echo "0x30 0x06 0x05 0x61 0x00 0xbc 0x20 0x04 0x00" | execute_cmd)

  echo "< Get biosscratchpad7[26]: DWR assert check >"
  echo "$RES"
  echo

  # Completion Code
  CC=$(echo $RES| awk '{print $1;}')
  DWR=0x$(echo $RES| awk '{print $5;}')

  # Success
  if [ "$CC" == "40" ]; then
    if [ $((DWR & 0x04)) == 4 ]; then
      echo "Auto Dump in DWR mode"
      return 2
    else
      echo "Auto Dump in non-DWR mode"
      return 1
    fi
  else
    echo "get DWR mode fail"
    return 0
  fi
}

if [ "$#" -eq 1 ] && [ $1 = "time" ]; then
  now=$(date)
  echo "Crash Dump generated at $now"
  exit 0
fi

if [ "$#" -ne 2 ]; then
  echo "$0 <slot#> coreid    ==> for CPU CoreID"
  echo "$0 <slot#> msr       ==> for CPU MSR"
  echo "$0 <slot#> sensors   ==> for sensor history"
  echo "$0 <slot#> threshold ==> for sensor threshold"
  echo "$0 <slot#> pcie      ==> for PCIe"
  echo "$0 <slot#> dwr       ==> for DWR check"
  exit 1
fi

echo ""

if [ "$2" = "coreid" ]; then
  [ -r $CMD_DIR/crashdump_coreid ] && \
      $ME_UTIL $1 --file $CMD_DIR/crashdump_coreid | sed 's/^57 01 00 //g'
elif [ "$2" = "msr" ]; then
  [ -r $CMD_DIR/crashdump_msr ] && \
      cat $CMD_DIR/crashdump_msr | execute_cmd
elif [ "$2" = "sensors" ]; then
  /usr/local/bin/sensor-util $1 --history $SENSOR_HISTORY \
   && /usr/local/bin/sensor-util spb --history $SENSOR_HISTORY \
   && /usr/local/bin/sensor-util nic --history $SENSOR_HISTORY
elif [ "$2" = "threshold" ]; then
  /usr/local/bin/sensor-util $1 --threshold \
   && /usr/local/bin/sensor-util spb --threshold \
   && /usr/local/bin/sensor-util nic --threshold
elif [ "$2" = "pcie" ]; then
  pcie_dump
elif [ "$2" = "dwr" ]; then
  dwr_dump
  exit $?
fi

exit 0
