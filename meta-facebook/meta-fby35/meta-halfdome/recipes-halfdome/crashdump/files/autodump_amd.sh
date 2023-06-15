#!/bin/bash

dump_sensor_history() 
{
  l_SLOT=$1
  l_SENSOR_HISTORY=180

  /usr/local/bin/sensor-util "$l_SLOT" --history $l_SENSOR_HISTORY \
   && /usr/local/bin/sensor-util bmc --history $l_SENSOR_HISTORY \
   && /usr/local/bin/sensor-util nic --history $l_SENSOR_HISTORY 
}

dump_sensor_threshold()
{
  l_SLOT=$1

  /usr/local/bin/sensor-util "$l_SLOT" --threshold \
   && /usr/local/bin/sensor-util bmc --threshold \
   && /usr/local/bin/sensor-util nic --threshold
}

SLOT_NAME=$1

case $SLOT_NAME in
  slot1)
    SLOT_NUM=1
    ;;
  slot2)
    SLOT_NUM=2
    ;;
  slot3)
    SLOT_NUM=3
    ;;
  slot4)
    SLOT_NUM=4
    ;;
  *)
    N=${0##*/}
    echo "Usage: $N {slot1|slot2|slot3|slot4}"
    exit 1
    ;;
esac

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE="/var/run/autodump${SLOT_NUM}.pid"
DUMP_DIR='/var/lib/amd-ras'
CURRENT_INDEX_FILE="$DUMP_DIR/fru${SLOT_NUM}_current_index"
INFODUMP_FILE="$DUMP_DIR/fru${SLOT_NUM}_crashdump_info"
DUMP_UTIL='/usr/bin/amd-ras'
FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"

# check crashdump binary file exists
if [ ! -f $DUMP_UTIL ]; then
  logger -t "ipmid" -p daemon.crit "AMD ADDC crashdump not supported"
  exit 1
fi

# check if auto crashdump is already running
if [ -f $PID_FILE ]; then
  logger -t "ipmid" -p daemon.warning "Another crashdump for $SLOT_NAME is runnung"
  exit 1
else
  touch $PID_FILE
fi

# Set crashdump timestamp
# this timestamp is used by pal_is_crashdump_ongoing() and it can
# prevent the power control when crashdump is ongoing
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" "$sys_runtime")
kv set "fru${SLOT_NUM}_crashdump" "$((sys_runtime+600))"

# check number of crashdump file exists
[ -r $CURRENT_INDEX_FILE ] && PREVIOUS_INDEX=$(cat $CURRENT_INDEX_FILE) || PREVIOUS_INDEX=0

CURT_DTIME=$(date +"%Y%m%d-%H%M%S")
DUMP_FILE="$DUMP_DIR/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}.cper"
LOG_ARCHIVE="/mnt/data/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}_${CURT_DTIME}.tar.gz"

logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM started"

#HEADER LINE for the dump
echo "Crash Dump generated at $(date)" > "$INFODUMP_FILE"
{
  # Get BMC version & hostname
  echo "Get BMC version info: "
  "$FW_UTIL" bmc --version
  uname -a
  cat /etc/issue

  # Get fw info

  echo "Get firmware version info: "
  "$FW_UTIL" "$SLOT_NAME" "--version"

  # Get FRUID info
  echo "Get FRUID Info:"
  "$FRUID_UTIL" "$SLOT_NAME"

  # Sensors & sensor thresholds
  echo "Sensor history at dump:"
  dump_sensor_history "$SLOT_NAME"
  echo "Sensor threshold at dump:"
  dump_sensor_threshold "$SLOT_NAME"
} >> "$INFODUMP_FILE"

RAS_STATUS=$2
NUM_OF_PROC=$3
TARGET_CPU=$4
CPUID_EAX=$5
CPUID_EBX=$6
CPUID_ECX=$7
CPUID_EDX=$8

$DUMP_UTIL --fru "$SLOT_NUM" --ras "$RAS_STATUS" --ncpu "$NUM_OF_PROC" --tcpu "$TARGET_CPU"  --bid 0 --cid "$CPUID_EAX" "$CPUID_EBX" "$CPUID_ECX" "$CPUID_EDX"

[ -r $CURRENT_INDEX_FILE ] && CURRENT_INDEX=$(cat $CURRENT_INDEX_FILE) || CURRENT_INDEX=0
if [ "$PREVIOUS_INDEX" == "$CURRENT_INDEX" ]; then
  DUMP_FILE=
fi

IFS=" " read -r -a LOGS <<< "$(basename "$INFODUMP_FILE") $(basename "$DUMP_FILE")"
tar zcf "$LOG_ARCHIVE" -C "$DUMP_DIR" "${LOGS[@]}" && rm -rf "$DUMP_FILE"
if [ -f "$LOG_ARCHIVE" ]; then
  logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM is generated at $LOG_ARCHIVE"
fi

# Remove current pid file
rm $PID_FILE
exit 0
