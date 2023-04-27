#!/bin/bash

amd_warm_reset() {
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFF
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFB
  sleep 0.5
  /usr/bin/bic-util "$SLOT_NAME" 0x18 0x52 0x00 0x42 0x00 0x00 0xFF
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

# check number of crashdump file exists
[ -r $CURRENT_INDEX_FILE ] && PREVIOUS_INDEX=$(cat $CURRENT_INDEX_FILE) || PREVIOUS_INDEX=0

DUMP_FILE="/var/lib/amd-ras/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}.cper"
LOG_ARCHIVE="/mnt/data/fru${SLOT_NUM}_ras-error${PREVIOUS_INDEX}.tar.gz"

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

RAS_STATUS=$2
NUM_OF_PROC=$3
TARGET_CPU=$4
CPUID_EAX=$5
CPUID_EBX=$6
CPUID_ECX=$7
CPUID_EDX=$8

$DUMP_UTIL --fru "$SLOT_NUM" --ras "$RAS_STATUS" --ncpu "$NUM_OF_PROC" --tcpu "$TARGET_CPU"  --bid 0 --cid "$CPUID_EAX" "$CPUID_EBX" "$CPUID_ECX" "$CPUID_EDX"

[ -r $CURRENT_INDEX_FILE ] && CURRENT_INDEX=$(cat $CURRENT_INDEX_FILE) || CURRENT_INDEX=0
if [ "$PREVIOUS_INDEX" != "$CURRENT_INDEX" ]; then
    tar zcf "$LOG_ARCHIVE" -C "$(dirname "$DUMP_FILE")" "$(basename "$DUMP_FILE")" && \
    rm -rf "$DUMP_FILE" && \
    logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM is generated at $LOG_ARCHIVE"
fi

# Remove current pid file
rm $PID_FILE

# 0 ---> warm
# 1 ---> cold
# 2 ---> no reset

if [ -r $CONFIG_FILE ]; then
  RESET_OPTION=$(tail -n 1 "${CONFIG_FILE}")
else
  RESET_OPTION=0
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