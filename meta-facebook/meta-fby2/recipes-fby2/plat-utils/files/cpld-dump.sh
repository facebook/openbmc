#!/bin/bash
. /usr/local/fbpackages/utils/ast-functions

SLOT_NAME=$1
CPLDDUMP_FILE="/mnt/data/cplddump_$SLOT_NAME"


function cpld_dump_process {
  server_type=$(get_server_type $1)
  if [ "$server_type" == "1" ]; then
    /usr/local/bin/dump_cpld_rc.sh $SLOT_NAME
  elif [ "$server_type" == "2" ]; then
    /usr/local/bin/dump_cpld_ep.sh $SLOT_NAME
  fi
}

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
      N=${N#[SK]??}
      echo "Usage: $N {slot1|slot2|slot3|slot4}"
      exit 1
      ;;
esac

# File format cplddump<slot_id>.pid (See pal_is_cplddump_ongoing()
# function definition)
PID_FILE="/var/run/cplddump$SLOT_NUM.pid"

# check if auto CPLD dump is already running
if [ -f $PID_FILE ]; then
  echo "Another auto CPLD dump for $SLOT_NAME is running"
  exit 1
else
  touch $PID_FILE
fi

# Set cplddump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+630)) > /tmp/cache_store/fru${SLOT_NUM}_cplddump

LOG_MSG_PREFIX=""

echo "Auto CPLD Dump for $SLOT_NAME Started"

#HEADER LINE for the dump
now=$(date)
echo "Auto CPLD Dump generated at $now" > $CPLDDUMP_FILE

cpld_dump_process $SLOT_NUM >> $CPLDDUMP_FILE

echo -n "Auto CPLD Dump End at " >> $CPLDDUMP_FILE
date >> $CPLDDUMP_FILE

logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}CPLD dump for FRU: $SLOT_NUM is generated at $CPLDDUMP_FILE"

echo "Auto CPLD Dump for $SLOT_NAME Completed"

# Remove current pid file
rm $PID_FILE

echo "${LOG_MSG_PREFIX}Auto CPLD Dump Stored in $CPLDDUMP_FILE"
exit 0
