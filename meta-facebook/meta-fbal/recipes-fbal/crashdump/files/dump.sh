#!/bin/bash

PROGRAM=$0
PECI_UTIL="/usr/local/bin/peci-util"
CMD_DIR="/etc/peci"

let CPU_ID=($1 - 48)
DUMP_FILE=crashdump_p"$CPU_ID"_coreid
MODE=$(($(/usr/bin/kv get mb_skt) >> 1))   #2S:MODE=2, 4S:MODE=1
if [ "$MODE" -eq 1 ]; then
  let MAX_CPU=4;
else
  let MAX_CPU=2;
fi

function print_help_msg {
  echo "$PROGRAM 48 coreid  ==> for CPU 1 CoreID"
  echo "$PROGRAM 49 coreid  ==> for CPU 2 CoreID"
  if [[ "$MODE" -eq 1 ]]; then
    echo "$PROGRAM 50 coreid  ==> for CPU 3 CoreID"
    echo "$PROGRAM 51 coreid  ==> for CPU 4 CoreID"
  fi
  echo "$PROGRAM 48 msr     ==> for CPU 1 MSR"
  echo "$PROGRAM 49 msr     ==> for CPU 2 MSR"
  if [[ "$MODE" -eq 1 ]]; then
    echo "$PROGRAM 50 msr     ==> for CPU 3 MSR"
    echo "$PROGRAM 51 msr     ==> for CPU 4 MSR"
  fi
  echo "$PROGRAM pcie       ==> for PCIe"
  echo "$PROGRAM dwr        ==> for DWR check"
}

# read command from stdin
function execute_via_peci {
  # if PECI_UTIL cannot be executed, return
  [ -x $PECI_UTIL ] || \
    return

  cat | \
    $PECI_UTIL -i 10 -r 5 --file /dev/stdin
}

# read command from stdin
function execute_cmd {
  # echo "Use BMC wired PECI interface"
  cat | execute_via_peci
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

function pcie_dump {
  # CPU and PCH
  [ -r $CMD_DIR/crashdump_pcie ] && \
    cat $CMD_DIR/crashdump_pcie | execute_cmd

  # Buses
  RES=$(echo "0x30 0x06 0x05 0x61 0x00 0xcc 0x20 0x04 0x00" | execute_cmd)
  echo "Get CPUBUSNO: $RES"
  # Completion Code
  CC=$(echo $RES| awk '{print $1;}')
  # Root port buses
  ROOT_BUSES=$(echo $RES | awk '{printf "0x%s 0x%s 0x%s", $3, $4, $5;}')

  # Success
  if [ "$CC" == "40" ]; then
    for ROOT in $ROOT_BUSES; do
      for DEV in {0..3}; do
        echo "Find Root Port Bus $ROOT Dev $DEV..."
        RES=$(echo "0x30 0x06 0x05 0x61 0x00 $(PCI_Config_Addr $ROOT $DEV 0x00 0x18)" | execute_cmd)
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
      done
    done
  fi
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

echo ""


if [ "$#" -eq 1 ] && [ $1 = "pcie" ]; then

  pcie_dump

  exit 0
fi

if [ "$#" -eq 1 ] && [ $1 = "dwr" ]; then

  dwr_dump

  exit $?
fi

if [ "$#" -ne 2 ]; then

  print_help_msg

  exit 1

elif [ "$CPU_ID" -ge "$MAX_CPU" ] || [ "$CPU_ID" -lt 0 ]; then

  print_help_msg

  exit 1

elif [ "$2" = "coreid" ]; then

  if [ -r "$CMD_DIR/$DUMP_FILE" ]; then
    cat $CMD_DIR/$DUMP_FILE | execute_cmd
  else
    echo "$CMD_DIR/$DUMP_FILE" not exist
  fi
  exit 0

elif [ "$2" = "msr" ]; then

  if [ -r "$CMD_DIR/$DUMP_FILE" ]; then
    cat $CMD_DIR/$DUMP_FILE | execute_cmd
  else
    echo "$CMD_DIR/$DUMP_FILE" not exist
  fi
  exit 0

fi

exit 0
