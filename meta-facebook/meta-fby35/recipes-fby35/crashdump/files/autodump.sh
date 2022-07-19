#!/bin/bash

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

BOARD_ID=$(get_bmc_board_id)
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
    if [ "$BOARD_ID" = "$BMC_ID_CLASS2" ]; then
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
PID_FILE="/var/run/autodump${SLOT_NUM}.pid"
DUMP_DIR='/tmp/crashdump/output'
PARSE_FILE="$DUMP_DIR/bafi_${SLOT_NAME}.log"
DUMP_FILE="$DUMP_DIR/crashdump_${SLOT_NAME}.log"
DUMP_UTIL='/usr/bin/crashdump'
PARSE_UTIL='/usr/bin/bafi'
LOG_ARCHIVE="/mnt/data/crashdump_${SLOT_NAME}.tar.gz"
ACD=true

if [ ! -f $DUMP_UTIL ]; then
  ACD=false
  DUMP_UTIL="/usr/bin/dump.sh $SLOT_NAME"
  mkdir -p $DUMP_DIR
fi

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=$(cat $PID_FILE) || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# kill previous autodump if exist
if [ -n "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null); then
  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  if [ "$ACD" = true ]; then
    pkill -KILL -f "$DUMP_UTIL $SLOT_NUM" >/dev/null 2>&1
    pkill -KILL -f "$PARSE_UTIL .*${SLOT_NAME}" >/dev/null 2>&1
  else
    pkill -KILL -f "$DUMP_UTIL" >/dev/null 2>&1
    pkill -KILL -f "me-util $SLOT_NAME" >/dev/null 2>&1
    pkill -KILL -f "bic-util $SLOT_NAME" >/dev/null 2>&1

    { echo -n "(uncompleted) Auto Dump End at "; date; } >> "$DUMP_FILE"
    tar zcf "/mnt/data/crashdump_uncompleted_${SLOT_NAME}.tar.gz" -C $DUMP_DIR "$(basename "$DUMP_FILE")"
  fi
fi
unset OLDPID

# Set crashdump timestamp
# this timestamp is used by pal_is_crashdump_ongoing() and it can
# prevent the power control when crashdump is ongoing
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" "$sys_runtime")
kv set "fru${SLOT_NUM}_crashdump" "$((sys_runtime+600))"

DELAY_SEC=30

while test $# -gt 1; do
  case "$2" in
  --now)
    DELAY_SEC=0
    ;;
  *)
    echo "unknown argument $2"
    ;;
  esac
  shift
done

if [ "$DELAY_SEC" != "0" ]; then
  echo "Auto Dump will start after ${DELAY_SEC}s..."
  sleep "$DELAY_SEC"
fi

echo "Auto Dump for $SLOT_NAME Started"
logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM started"

LOGS=("$(basename "$DUMP_FILE")")
if [ "$ACD" = true ]; then
  $DUMP_UTIL $SLOT_NUM

  # shellcheck disable=SC2012
  LOG_FILE=$(ls $DUMP_DIR/crashdump_"${SLOT_NAME}"* -t1 |head -n 1)
  $PARSE_UTIL "$LOG_FILE" > "$PARSE_FILE" 2>&1

  LOGS=("$(basename "$LOG_FILE")" "$(basename "$PARSE_FILE")")
else
  { echo -n "Auto Dump Start at "; date; } > "$DUMP_FILE"
  { $DUMP_UTIL coreid; $DUMP_UTIL msr; $DUMP_UTIL pcie; } >> "$DUMP_FILE" 2>&1
  { echo -n "Auto Dump End at "; date; } >> "$DUMP_FILE"
fi

tar zcf "$LOG_ARCHIVE" -C $DUMP_DIR "${LOGS[@]}"
logger -t "ipmid" -p daemon.crit "Crashdump for FRU: $SLOT_NUM is generated at $LOG_ARCHIVE"

# Remove current pid file
rm $PID_FILE

echo "Auto Dump for $SLOT_NAME Stored in $LOG_ARCHIVE"
exit 0
