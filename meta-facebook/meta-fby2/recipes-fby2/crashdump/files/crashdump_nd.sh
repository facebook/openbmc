#!/bin/bash

FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"
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
      N=${N#[SK]??}
      echo "Usage: $N {slot1|slot2|slot3|slot4}"
      exit 1
      ;;
esac

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE="/var/run/autodump${SLOT_NUM}.pid"

DUMP_SCRIPT="/usr/local/bin/dump.sh"
CRASHDUMP_FILE="/tmp/crashdump_${SLOT_NAME}"
CRASHDUMP_DECODED_FILE="/tmp/crashdump_${SLOT_NAME}_mca"
CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_${SLOT_NAME}.tar.gz"
LOG_MSG_PREFIX=""

echo "Auto Dump for $SLOT_NAME Started"
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM started"

#HEADER LINE for the dump
$DUMP_SCRIPT "time" > "$CRASHDUMP_FILE"

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
  "$DUMP_SCRIPT" "$SLOT_NAME" "sensors"
  echo "Sensor threshold at dump:"
  "$DUMP_SCRIPT" "$SLOT_NAME" "threshold"

  # MCA dumps
  echo "MCA(x) Dump:"
  cat "$CRASHDUMP_DECODED_FILE"
  echo "================="

  echo -n "Auto Dump End at "
  date
} >> "$CRASHDUMP_FILE"

tar zcf "$CRASHDUMP_LOG_ARCHIVE" -C "$(dirname "$CRASHDUMP_FILE")" \
        "$(basename "$CRASHDUMP_FILE")" && \
rm -rf "$CRASHDUMP_DECODED_FILE" && \
rm -rf "$CRASHDUMP_FILE" && \
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM is generated at $CRASHDUMP_LOG_ARCHIVE"
cp -f "$CRASHDUMP_LOG_ARCHIVE" /tmp
echo "Auto Dump for $SLOT_NAME Completed"

# Remove current pid file
rm "$PID_FILE"

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $CRASHDUMP_LOG_ARCHIVE"
exit 0
