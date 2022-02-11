#!/bin/bash

SLOT_NAME=$1

case $SLOT_NAME in
    scm)
      SLOT_NUM=1
      ;;
    *)
      N=${0##*/}
      N=${N#[SK]??}
      echo "Usage: $N {scm}"
      exit 1
      ;;
esac

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE="/var/run/autodump$SLOT_NUM.pid"

# check if auto crashdump is already running
if [ -f $PID_FILE ]; then
  echo "Another auto crashdump for $SLOT_NAME is runnung"
  exit 1
else
  touch $PID_FILE
fi

# Set crashdump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+630)) > /tmp/cache_store/fru${SLOT_NUM}_crashdump

DUMP_SCRIPT="/usr/local/bin/dump.sh"
CRASHDUMP_FILE="/mnt/data/crashdump_$SLOT_NAME"
CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_$SLOT_NAME.tar.gz"

echo "Auto Dump for $SLOT_NAME Started"

#HEADER LINE for the dump
$DUMP_SCRIPT "time" > $CRASHDUMP_FILE
#COREID dump
$DUMP_SCRIPT $SLOT_NAME "coreid" >> $CRASHDUMP_FILE
#MSR dump
$DUMP_SCRIPT $SLOT_NAME "msr" >> $CRASHDUMP_FILE
# Sensors & sensor thresholds
echo "Sensor history at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "sensors" >> $CRASHDUMP_FILE
echo "Sensor threshold at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "threshold" >> $CRASHDUMP_FILE

tar zcf $CRASHDUMP_LOG_ARCHIVE -C `dirname $CRASHDUMP_FILE` `basename $CRASHDUMP_FILE` && \
rm -rf $CRASHDUMP_FILE && \
logger -t "ipmid" -p daemon.crit "Crashdump for $SLOT_NAME is generated at $CRASHDUMP_LOG_ARCHIVE"

echo "Auto Dump for $SLOT_NAME Completed"

rm $PID_FILE
exit 0
