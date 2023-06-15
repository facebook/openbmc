#!/bin/bash

PID=$$
# File format autodump<fru>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE='/var/run/autodump1.pid'
DUMP_DIR='/var/lib/amd-ras'
CURRENT_INDEX_FILE="$DUMP_DIR/fru1_current_index"
INFODUMP_FILE="$DUMP_DIR/sensordump.log"
DUMP_UTIL='/usr/bin/amd-ras'
LOG_ARCHIVE='/mnt/data/autodump.tar.gz'

SENSOR_HISTORY=180

# check crashdump binary file exists
if [ ! -f $DUMP_UTIL ]; then
  logger -t "ipmid" -p daemon.crit "AMD ADDC crashdump not supported"
  exit 1
fi

# check if auto crashdump is already running
if [ -f $PID_FILE ]; then
  logger -t "ipmid" -p daemon.warning "Another crashdump is runnung"
  exit 1
else
  echo $PID > $PID_FILE
fi

# Set crashdump timestamp
# this timestamp is used by pal_is_crashdump_ongoing() and it can
# prevent the power control when crashdump is ongoing
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" "$sys_runtime")
kv set fru1_crashdump $((sys_runtime+300))

# check number of crashdump file exists
[ -r $CURRENT_INDEX_FILE ] && PREVIOUS_INDEX=$(cat $CURRENT_INDEX_FILE) || PREVIOUS_INDEX=0
DUMP_FILE="$DUMP_DIR/fru1_ras-error${PREVIOUS_INDEX}.cper"

for i in {0..1}; do
  if ! id[$i]=$(kv get cpu"${i}"_id persistent); then
    id[$i]="0 0 0 0"
  fi
done
IFS=" " read -r -a CPUID <<< "${id[@]}"

echo "Auto Dump Started"
{ echo -n "Sensor Dump Start at "; date; } > $INFODUMP_FILE

# Sensors
{
  echo ""
  echo "Sensor history of last ${SENSOR_HISTORY}s at dump:"
  /usr/local/bin/sensor-util all --history $SENSOR_HISTORY

  echo "Sensor threshold at dump:"
  /usr/local/bin/sensor-util all --threshold

  echo -n "Dump End at "; date
} >> $INFODUMP_FILE 2>&1

$DUMP_UTIL --fru 1 --ncpu 2 --cid "${CPUID[@]}"

[ -r $CURRENT_INDEX_FILE ] && CURRENT_INDEX=$(cat $CURRENT_INDEX_FILE) || CURRENT_INDEX=0
if [ "$PREVIOUS_INDEX" == "$CURRENT_INDEX" ]; then
  DUMP_FILE=
fi

IFS=" " read -r -a LOGS <<< "$(basename "$INFODUMP_FILE") $(basename "$DUMP_FILE")"
tar zcf $LOG_ARCHIVE -C $DUMP_DIR "${LOGS[@]}"
logger -t "ipmid" -p daemon.crit "Crashdump for FRU: 1 is generated at $LOG_ARCHIVE"

# Remove current pid file
rm $PID_FILE

echo "Auto Dump Stored in $LOG_ARCHIVE"
exit 0
