#!/bin/bash

SLOT=$1
SLOT_NUM=${SLOT#"slot"}

PWR_UTIL_PID=$(ps | grep power-util | grep "$SLOT" | awk '{print $1}')
HSV_REINIT_PID=$(ps | grep hotservice-reinit.sh | grep "$SLOT" | awk '{print $1}')

PWR_UTIL_LOCK="/var/run/power-util_$SLOT_NUM.lock"
BIC_CACHED_LOCK="/var/run/bic-cached_$SLOT_NUM.lock"

if [ ! -z "$PWR_UTIL_PID" ]; then
  logger -p user.info "slot-removal-precheck.sh: kill power-util $SLOT, pid: $PWR_UTIL_PID"
  kill -9 "$PWR_UTIL_PID"
fi

if [ ! -z "$HSV_REINIT_PID" ]; then
  logger -p user.info "slot-removal-precheck.sh: kill hotservice-reinit.sh $SLOT, pid: $HSV_REINIT_PID"
  kill -9 "$HSV_REINIT_PID"
fi

# check the lock file to find out the flock are occupied by which process
# then kill the processes to free the flock
for lock_file in "$PWR_UTIL_LOCK" "$BIC_CACHED_LOCK"
do
  if [ -f "$lock_file" ]; then
    for lock_pid in $(fuser "$lock_file")
    do
      cmdline=$(cat < /proc/"$lock_pid"/cmdline | tr '\000' ' ')
      logger -p user.info "slot-removal-precheck.sh: kill $cmdline, pid: $lock_pid, lock_file: $lock_file"
      kill -9 "$lock_pid"
    done

    rm "$lock_file"
  fi
done
