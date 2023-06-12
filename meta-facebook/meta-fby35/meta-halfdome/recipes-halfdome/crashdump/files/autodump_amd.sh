#!/bin/bash

amd_warm_reset() {
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFF
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFB
  sleep 0.5
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFF
}

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
CURRENT_INDEX_FILE="/var/lib/amd-ras/fru${SLOT_NUM}_current_index"
DUMP_UTIL='/usr/bin/amd-ras'
CONFIG_FILE="/var/lib/amd-ras/config_file"
FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"

# check number of crashdump file exists
[ -r $CURRENT_INDEX_FILE ] && PREVIOUS_INDEX=$(cat $CURRENT_INDEX_FILE) || PREVIOUS_INDEX=0

CURT_DTIME=$(date +"%Y%m%d-%H%M%S")
INFODUMP_FILE="/var/lib/amd-ras/fru${SLOT_NUM}_crashdump_info"
DUMP_FILE="/var/lib/amd-ras/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}.cper"
LOG_ARCHIVE="/mnt/data/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}_${CURT_DTIME}.tar.gz"

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
if [ "$PREVIOUS_INDEX" != "$CURRENT_INDEX" ] || [ -f $INFODUMP_FILE ]; then
    tar zcf "$LOG_ARCHIVE" -C "$(dirname "$DUMP_FILE")" "$(basename "$DUMP_FILE")" "$(basename "$INFODUMP_FILE")" && \
    rm -rf "$DUMP_FILE"
    if [ -f "$LOG_ARCHIVE" ];then
      logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM is generated at $LOG_ARCHIVE"
    fi
fi

# Remove current pid file
rm $PID_FILE

# 0 ---> warm
# 1 ---> cold
# 2 ---> no reset

if [ -r $CONFIG_FILE ]; then
  RESET_OPTION=$(/usr/bin/python -m json.tool "${CONFIG_FILE}" | grep "systemRecovery" | awk -F ': ' '{print $2}')
else
  RESET_OPTION=0
fi

if [ "$RESET_OPTION" -eq 0 ]; then
  FCH_ERR=$((RAS_STATUS & 0x02))
  RST_CTRL_ERR=$((RAS_STATUS & 0x04))
  #Force cold reset to recover system
  if [ "$FCH_ERR" -ne 0 ] || [ "$RST_CTRL_ERR" -ne 0 ];then
    RESET_OPTION=1
  fi
fi

case $RESET_OPTION in
  0)
    logger -t "ipmid" -p daemon.crit "Dump completed, warm reset FRU: $SLOT_NUM"
    amd_warm_reset
    ;;
  1)
    logger -t "ipmid" -p daemon.crit "Dump completed, cold reset FRU: $SLOT_NUM"
    /usr/local/bin/power-util "$SLOT_NAME" reset
    ;;
  2)
    logger -t "ipmid" -p daemon.crit "Dump completed, no reset FRU: $SLOT_NUM"
    ;;
  *)
    logger -t "ipmid" -p daemon.crit "Dump completed, Crashdump Reset Policy is not valid"
    ;;
esac

exit 0