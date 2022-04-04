#!/bin/bash
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"
KV_CMD="/usr/bin/kv"
SLOT_NAME=""
RUN_MODE="manual"

SCRIPT_PATH="${0}"

update_crashdump_history()
{
  l_SLOT_NUM=$1
  l_CURT_DTIME=$2

  HISTORY_KEY="crashdump_history_slot$l_SLOT_NUM"

  HISTORY_RECORD=$($KV_CMD get "$HISTORY_KEY" persistent)
  HIST_INDEX_1=$(echo "$HISTORY_RECORD" | awk -F',' '{ print $1 }')
  HIST_INDEX_2=$(echo "$HISTORY_RECORD" | awk -F',' '{ print $2 }')

  OUT_STR="$l_CURT_DTIME"
  if [ ! -z "$HIST_INDEX_1" ]; then
    OUT_STR="$OUT_STR,$HIST_INDEX_1"
  fi
  if [ ! -z "$HIST_INDEX_2" ]; then
    OUT_STR="$OUT_STR,$HIST_INDEX_2"
  fi
 
  $KV_CMD set "$HISTORY_KEY" "$OUT_STR" persistent
}

clear_crashdump_archive()
{
  l_SLOT_NUM=$1
  HISTORY_KEY="crashdump_history_slot$l_SLOT_NUM"

  HISTORY_RECORD=$(${KV_CMD} get "$HISTORY_KEY" persistent)
  hist_dtime_1=$(echo "$HISTORY_RECORD" | awk -F',' '{ print $1 }')
  hist_dtime_2=$(echo "$HISTORY_RECORD" | awk -F',' '{ print $2 }')
  hist_dtime_3=$(echo "$HISTORY_RECORD" | awk -F',' '{ print $3 }')

  for i in /mnt/data/crashdump_slot"$l_SLOT_NUM"*.tar.gz
  do
    file_dtime=$(basename "$i" | awk -F'.' '{print $1}' | awk -F'_' '{print $3}')
    if [ "$file_dtime" = "$hist_dtime_1" ]; then
      continue
    elif [ "$file_dtime" = "$hist_dtime_2" ]; then
      continue
    elif [ "$file_dtime" = "$hist_dtime_3" ]; then
      continue
    else
      rm "$i"
    fi
  done
}

dump_sensor_history() 
{
  l_SLOT=$1
  l_SENSOR_HISTORY=180

  /usr/local/bin/sensor-util "$l_SLOT" --history $l_SENSOR_HISTORY \
   && /usr/local/bin/sensor-util spb --history $l_SENSOR_HISTORY \
   && /usr/local/bin/sensor-util nic --history $l_SENSOR_HISTORY 
}

dump_sensor_threshold()
{
  l_SLOT=$1

  /usr/local/bin/sensor-util "$l_SLOT" --threshold \
   && /usr/local/bin/sensor-util spb --threshold \
   && /usr/local/bin/sensor-util nic --threshold
}

script_filename()
{
  N=${SCRIPT_PATH##*/}
  N=${N#[SK]??}
  echo "$N"
}

for i in "$@"; do
  case "$1" in
    -e|--event)
      RUN_MODE="event"
      shift
      ;;
    -*|--*=) # unsupported flags
      echo "Error: Unsupported flag $i" >&2
      exit 1
      ;;
    *) # preserve positional arguments
      SLOT_NAME="$i"
      shift
      ;;
  esac
done

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
      echo "Usage: $(script_filename) {slot1|slot2|slot3|slot4}"
      exit 1
      ;;
esac

# We only support event triggered crashdump for now
if [ "$RUN_MODE" = "manual" ]; then
  echo "$(script_filename): Manual trigger not supported"
  exit 1
fi

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE="/var/run/autodump${SLOT_NUM}.pid"

CURT_DTIME=$(date +"%Y%m%d-%H%M%S")
CRASHDUMP_FILE="/tmp/crashdump_${SLOT_NAME}"
CRASHDUMP_DECODED_FILE="/tmp/crashdump_${SLOT_NAME}_mca"
CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_${SLOT_NAME}_${CURT_DTIME}.tar.gz"
LOG_MSG_PREFIX=""

echo "Auto Dump for $SLOT_NAME Started"
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM started"

#HEADER LINE for the dump
echo "Crash Dump generated at $(date)" > "$CRASHDUMP_FILE"

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

  # MCA dumps
  echo "Crashdump v1.0:"
  cat "$CRASHDUMP_DECODED_FILE"
  echo "================="

  echo "Auto Dump End at $(date)"
} >> "$CRASHDUMP_FILE"

tar zcf "$CRASHDUMP_LOG_ARCHIVE" -C "$(dirname "$CRASHDUMP_FILE")" "$(basename "$CRASHDUMP_FILE")" && \
rm -rf "$CRASHDUMP_DECODED_FILE" && \
rm -rf "$CRASHDUMP_FILE" && \
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM is generated at $CRASHDUMP_LOG_ARCHIVE"
# cp -f "$CRASHDUMP_LOG_ARCHIVE" /tmp

# update crashdump history, only keep last three archive for each slot
update_crashdump_history $SLOT_NUM "$CURT_DTIME"
clear_crashdump_archive $SLOT_NUM

echo "Auto Dump for $SLOT_NAME Completed"

# Remove current pid file
rm "$PID_FILE"

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $CRASHDUMP_LOG_ARCHIVE"
exit 0
