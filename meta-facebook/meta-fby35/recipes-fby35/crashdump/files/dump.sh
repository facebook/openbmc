#!/bin/bash

ME_UTIL="/usr/local/bin/me-util"
PECI_UTIL="/usr/bin/bic-util"
CMD_DIR="/usr/bin"
INTERFACE="PECI_INTERFACE"
SENSOR_UTIL="/usr/local/bin/sensor-util"
SENSOR_HISTORY=180
SLOT=$1

# read command from stdin
function execute_via_me {
  # if ME_UTIL cannot be executed, return
  [ -x $ME_UTIL ] || return

  # Adapt command to ME-UTIL format
  cat | sed 's/^0x/0xb8 0x40 0x57 0x01 0x00 0x/g' | \
    $ME_UTIL "$SLOT" --file /dev/stdin | sed 's/^57 01 00 //g'
}

# read command from stdin
function execute_via_peci {
  # if PECI_UTIL cannot be executed, return
  [ -x $PECI_UTIL ] || return

  cat | sed 's/^0x/0xe0 0x29 0x15 0xa0 0x00 0x/g' | \
    $PECI_UTIL "$SLOT" --file /dev/stdin | sed 's/^15 A0 00 //g'
}

function execute_cmd {
  if [ "$INTERFACE" == "ME_INTERFACE" ]; then
    cat | execute_via_me
  else
    cat | execute_via_peci
  fi
}

# ARG: Bus[8], Device[5], Function[3], Register[12] (unit: bits)
# OUT: PCI config address
function PCI_Config_Addr {
  # BUS
  RESULT=$((($1 & 0xFF) << 20))
  # DEV
  RESULT=$((RESULT | (($2 & 0x1F) << 15)))
  # FUN
  RESULT=$((RESULT | (($3 & 0x7) << 12)))
  # REG
  RESULT=$((RESULT | (($4 & 0xFFF) << 0)))
  printf "0x%x 0x%x 0x%x 0x%x" \
    $((RESULT & 0xFF)) \
    $(((RESULT >> 8) & 0xFF)) \
    $(((RESULT >> 16) & 0xFF)) \
    $(((RESULT >> 24) & 0xFF))
  return 0
}

function print_bus_device {
  MAX_FUN=0
  MAX_DEV=4
  BUS=$(printf "0x%02x" "$1")
  BUSM=$(($1 >> 4))
  BUSL=$((($1 & 0xF) << 4))
  for (( DEV=0; DEV<=MAX_DEV; DEV++ )); do
    DEVM=$((DEV >> 1))
    DEVL=$(((DEV & 0x1) << 3))
    for (( FUN=0; FUN<=MAX_FUN; FUN++ )); do
      printf "echo \"BUS 0x%02x, Dev 0x%02x, Fun 0x%02x, Reg: 0x100 ~ 0x147\"\n" "$1" $DEV $FUN
      BYTE1=$(((DEVL | FUN) << 4 | 0x1))
      BYTE2=$((BUSL | DEVM))
      for REG in {0..68..4}; do
        printf "0x30 0x06 0x05 0x61 0x00 0x%02x 0x%02x 0x%02x 0x0%x\n" $REG $BYTE1 $BYTE2 $BUSM
      done
    done
  done
}

function find_end_device {
  echo "Find Root Port Bus $1 Dev $2 Fun $3..."
  RES=$(echo 0x30 0x06 0x05 0x61 0x00 "$(PCI_Config_Addr "$1" "$2" "$3" 0x18)" | execute_cmd)
  echo "$RES"
  CC=$(echo "$RES"| awk '{print $1;}')
  SECONDARY_BUS=0x$(echo "$RES"| awk '{print $3;}')
  SUBORDINATE_BUS=0x$(echo "$RES"| awk '{print $4;}')
  if [ "$CC" == "40" ] && [ "$SECONDARY_BUS" != "0x00" ] && [ "$SUBORDINATE_BUS" != "0x00" ]; then
    for (( BUS=SECONDARY_BUS; BUS<=SUBORDINATE_BUS; BUS++ )); do
      printf "End Device on Bus 0x%02x\n" $BUS
      print_bus_device $BUS | execute_cmd
    done
  fi
}

function pcie_dump {
  # CPU and PCH
  [ -r $CMD_DIR/crashdump_pcie ] && execute_cmd < $CMD_DIR/crashdump_pcie

  # Buses
  RES=$(echo "0x30 0x06 0x05 0x61 0x00 0xcc 0x20 0x04 0x00" | execute_cmd)
  echo "Get CPUBUSNO: $RES"

  # Completion Code
  CC=$(echo "$RES"| awk '{print $1;}')

  # Success
  if [ "$CC" == "40" ]; then
    # CPU root port - Bus 0x16/0x64, Dev 0~3, Fun:0
    # Root port buses
    ROOT_BUSES=$(echo "$RES" | awk '{printf "0x%s 0x%s", $3, $4;}')
    ROOT_BUS=$(echo "$RES" | awk '{printf "0x%s", $2;}')
    echo ROOT_BUSES="$ROOT_BUSES"
    for ROOT in $ROOT_BUSES; do
      for DEV in {0..3}; do
        find_end_device "$ROOT" "$DEV" 0
      done
    done

    # CPU root port - Bus 0,Dev 0, Fun:0
    echo ROOT_BUS="$ROOT_BUS"
    find_end_device "$ROOT_BUS" 0 0

    # PCH root port - Bus 0,Dev 0x1B, Fun:0
    find_end_device "$ROOT_BUS" 0x1B 0

    # PCH root port - Bus 0,Dev 0x1D, Fun:0
    find_end_device "$ROOT_BUS" 0x1D 0

    # PCH root port - Bus 0,Dev 0x1D, Fun:4
    find_end_device "$ROOT_BUS" 0x1D 4
  fi
}

function print_dump_help {
  N=${0##*/}
  echo "Usage:"
  echo "$N <slot#> coreid <INTERFACE> ==> for CPU CoreID"
  echo "$N <slot#> msr    <INTERFACE> ==> for CPU MSR"
  echo "$N <slot#> pcie   <INTERFACE> ==> for PCIe"
  echo "$N <slot#> sensors            ==> for sensor history"
  echo "$N <slot#> threshold          ==> for sensor threshold"
  exit 1
}

if [ "$#" -eq 1 ] && [ "$1" = "time" ]; then
  now=$(date)
  echo "Crash Dump generated at $now"
  exit 0
fi

if [ "$#" -eq 2 ] || [ "$#" -eq 3 ]; then
  if [ "$#" -eq 3 ]; then
    if [ "$3" != "ME_INTERFACE" ] && [ "$3" != "PECI_INTERFACE" ]; then
      print_dump_help
    fi
    INTERFACE="$3"
  fi

  case "$2" in
    coreid)
      [ -r $CMD_DIR/crashdump_coreid ] && execute_cmd < $CMD_DIR/crashdump_coreid
      ;;
    msr)
      [ -r $CMD_DIR/crashdump_msr ] && execute_cmd < $CMD_DIR/crashdump_msr
      ;;
    pcie)
      pcie_dump
      ;;
    sensors)
      "$SENSOR_UTIL" "$SLOT" --history "$SENSOR_HISTORY" && \
      "$SENSOR_UTIL" bmc --history "$SENSOR_HISTORY" && \
      "$SENSOR_UTIL" nic --history "$SENSOR_HISTORY"
      ;;
    threshold)
      "$SENSOR_UTIL" "$SLOT" --threshold && \
      "$SENSOR_UTIL" bmc --threshold && \
      "$SENSOR_UTIL" nic --threshold
      ;;
    *)
      print_dump_help
      ;;
  esac
else
  print_dump_help
fi

exit 0
