#!/bin/bash
. /usr/local/fbpackages/utils/ast-functions

BOARD_ID=$(get_bmc_board_id)
SLOT_NAME=$1
PCIE_INTERFACE="PECI_INTERFACE"

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
      if [ "$BOARD_ID" = "9" ]; then
        echo "Usage: $N {slot1}"
      else
        echo "Usage: $N {slot1|slot2|slot3|slot4}"
      fi
      exit 1
      ;;
esac


if [ "$BOARD_ID" = "$BMC_ID_CLASS2" ] && [ "$SLOT_NAME" != "slot1" ]; then
  # class2 checking
  echo "$SLOT_NAME not supported"
  exit 1
fi

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID=$$
PID_FILE="/var/run/autodump$SLOT_NUM.pid"

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=$(cat $PID_FILE) || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# kill previous autodump if exist
if [ -n "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null) ; then
  # Check if 2nd dump is running
  if [ -n "$(ps | grep '{autodump.sh}' | grep "${SLOT_NAME}" | grep "second\|dwr")" ]; then
    echo $OLDPID > $PID_FILE
    echo "2nd DUMP or Demoted Warm Reset DUMP is running. exit."
    exit 1
  fi

  LOG_FILE="/tmp/autodump.log"
  LOG_ARCHIVE="/mnt/data/autodump_uncompleted_${SLOT_NAME}.tar.gz"
  echo -n "(uncompleted) Auto Dump End at " >> $LOG_FILE
  date >> $LOG_FILE

  tar zcf "$LOG_ARCHIVE" -C "$(dirname $LOG_FILE)" "$(basename $LOG_FILE)" && \
  rm -rf $LOG_FILE && \

  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  ps | grep '{dump.sh}' | grep "$SLOT_NAME" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
  ps | grep 'me-util' | grep "$SLOT_NAME" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
  ps | grep 'bic-util' | grep "$SLOT_NAME" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
  ps | grep 'fw-util' | grep "$SLOT_NAME" | grep "version" | awk '{print $1}'| xargs kill -s 9 &> /dev/null
fi
unset OLDPID

# Set crashdump timestamp
# this timestamp is used by pal_is_crashdump_ongoing() and it can prevent the power control when crashdump is ongoing.
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" "$sys_runtime")
kv set "fru${SLOT_NUM}_crashdump" "$((sys_runtime+1200))"

DUMP_SCRIPT="/usr/local/bin/dump.sh"
CRASHDUMP_FILE="/mnt/data/crashdump_$SLOT_NAME"
CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_$SLOT_NAME.tar.gz"
LOG_MSG_PREFIX=""

DWR=0
SECOND_DUMP=0
DELAY_SEC=30

while test $# -gt 1
do
  case "$2" in
  --dwr)
    DWR=1
    ;;
  --now)
    DELAY_SEC=0
    ;;
  --second)
    SECOND_DUMP=1
    ;;
  *)
    echo "unknown argument $2"
    ;;
  esac
  shift
done

if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
  echo "ACD Dump after System Reset or Demoted Warm Reset"
fi

if [ "$DELAY_SEC" != "0" ]; then
  echo "ACD Dump will start after ${DELAY_SEC}s..."

  sleep ${DELAY_SEC}
fi

echo "ACD Dump for $SLOT_NAME Started"
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM started"

/usr/bin/crashdump $SLOT_NUM

# only second/dwr autodump need to rename accordingly
if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
  # dwr
  $DUMP_SCRIPT "$SLOT_NAME"  "dwr" $PCIE_INTERFACE >> "$CRASHDUMP_FILE"

  # rename the archieve file based on whether dump in DWR mode or not
  if [ "$?" == "2" ]; then
    CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_dwr_$SLOT_NAME.tar.gz"
    LOG_MSG_PREFIX="DWR "
  else
    CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_second_$SLOT_NAME.tar.gz"
    LOG_MSG_PREFIX="SECOND_DUMP "
  fi
fi

echo -n "ACD Dump End at " >> "$CRASHDUMP_FILE"
date >> "$CRASHDUMP_FILE"

tar zcf "$CRASHDUMP_LOG_ARCHIVE" -C "$(dirname "$CRASHDUMP_FILE")" "$(basename "$CRASHDUMP_FILE")" && \
rm -rf "$CRASHDUMP_FILE" && \
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM is generated at $CRASHDUMP_LOG_ARCHIVE"
cp -f "$CRASHDUMP_LOG_ARCHIVE" /tmp
echo "ACD Dump for $SLOT_NAME Completed"

# Remove current pid file
rm $PID_FILE

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $CRASHDUMP_LOG_ARCHIVE"
exit 0
