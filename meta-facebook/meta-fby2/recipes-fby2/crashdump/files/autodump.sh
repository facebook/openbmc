#!/bin/bash

#
# Copyright 2014-present Facebook. All Rights Reserved.
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
#

ME_UTIL="/usr/local/bin/me-util"
BIC_UTIL="/usr/bin/bic-util"
FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"
SLOT_NAME=$1
INTERFACE="PECI_INTERFACE"

function is_numeric {
  if [ $(echo "$1" | grep -cE "^\-?([[:xdigit:]]+)(\.[[:xdigit:]]+)?$") -gt 0 ]; then
    return 1
  else
    return 0
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

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE="/var/run/autodump$SLOT_NUM.pid"

# check if auto crashdump is already running
if [ -f $PID_FILE ]; then
  echo "Another auto crashdump for $SLOT_NAME is running"
  exit 1
else
  touch $PID_FILE
fi

# Set crashdump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
/usr/bin/kv set fru${SLOT_NUM}_crashdump $((sys_runtime+1200))

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
  echo "Auto Dump after System Reset or Demoted Warm Reset"
fi

if [ "$DELAY_SEC" != "0" ]; then
  echo "Auto Dump will start after ${DELAY_SEC}s..."

  sleep ${DELAY_SEC}
fi

echo "Auto Dump for $SLOT_NAME Started"
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM started"

#HEADER LINE for the dump
$DUMP_SCRIPT "time" > $CRASHDUMP_FILE

# Get BMC version & hostname
strings /dev/mtd0 | grep 2016.07 >> $CRASHDUMP_FILE
uname -a >> $CRASHDUMP_FILE
cat /etc/issue >> $CRASHDUMP_FILE

# Get fw info
echo "Get firmware version info: " >> "$CRASHDUMP_FILE"
RES=$("$FW_UTIL" "$SLOT_NAME" "--version")
echo "$RES" >> "$CRASHDUMP_FILE"

# Get FRUID info
echo "Get FRUID Info:" >> $CRASHDUMP_FILE
RES=$($FRUID_UTIL $SLOT_NAME)
echo "$RES" >> $CRASHDUMP_FILE

# Get Device ID
echo "Get Device ID:" >> $CRASHDUMP_FILE
RES=$($ME_UTIL $SLOT_NAME 0x18 0x01)
echo "$RES" >> $CRASHDUMP_FILE

echo "Get CPU Microcode Update Revision:" >> "$CRASHDUMP_FILE"
RES="$($BIC_UTIL $SLOT_NAME 0xe0 0x29 0x15 0xa0 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x04 0x00)"
echo "$RES" >> "$CRASHDUMP_FILE"
RET=$?

if [ "$RET" -eq "0" ] && [ "${RES:0:11}" == "15 A0 00 40" ]; then
  echo "Use BIC wired PECI interface"
  INTERFACE="PECI_INTERFACE"
else
  echo "Use PECI through ME interface due to BIC wired PECI abnormal"
  INTERFACE="ME_INTERFACE"
fi

# Major Firmware Revision
REV=$(echo $RES| awk '{print $3;}')
# Check whether the first parameter is numeric or not
# If not, then it is an error message from me-util
is_numeric $(echo $RES| awk '{print $1;}')
if [ "$?" == 1 ] ;then
  Mode=$(echo $(($REV & 0x80)))
  if [ $Mode -ne 0 ] ;then
    echo "Device firmware update or Self-initialization in progress or Firmware in the recovery boot-loader mode" >> $CRASHDUMP_FILE
  fi
  # Get Self-test result
  echo "Get Self-test result:" >> $CRASHDUMP_FILE
  RES=$($ME_UTIL $SLOT_NAME 0x18 0x04)
  echo "$RES" >> $CRASHDUMP_FILE
  CC=$(echo $RES| awk '{print $1;}')
  # Check whether the first parameter is numeric or not
  # If not, then it is an error message from me-util
  is_numeric $CC
  if [ "$?" == 1 ] ;then
    if [ $CC -ne 55 ] ;then
      if [ $CC -eq 56 ] ;then
        echo "Self Test function not implemented in this controller" >> $CRASHDUMP_FILE
      elif [ $CC -eq 57 ] ;then
        echo "Corrupted or inaccessible data or devices" >> $CRASHDUMP_FILE
      elif [ $CC -eq 58 ] ;then
        echo "Fatal hardware error" >> $CRASHDUMP_FILE
      elif [ $CC -eq 80 ] ;then
        echo "PSU Monitoring service error" >> $CRASHDUMP_FILE
      elif [ $CC -eq 81 ] ;then
        echo "Firmware entered Recovery boot-loader mode" >> $CRASHDUMP_FILE
      elif [ $CC -eq 82 ] ;then
        echo "HSC Monitoring service error" >> $CRASHDUMP_FILE
      elif [ $CC -eq 83 ] ;then
        echo "Firmware entered non-UMA restricted mode of operation" >> $CRASHDUMP_FILE
      else
        echo "Unknown error" >> $CRASHDUMP_FILE
      fi
    fi
  fi
fi

# Sensors & sensor thresholds
echo "Sensor history at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "sensors" >> $CRASHDUMP_FILE
echo "Sensor threshold at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "threshold" >> $CRASHDUMP_FILE

echo COREID dump
date
echo "Dumping coreid.."
"$DUMP_SCRIPT" "$SLOT_NAME" "coreid" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping pcu.."
"$DUMP_SCRIPT" "$SLOT_NAME" "pcu" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping ubox.."
"$DUMP_SCRIPT" "$SLOT_NAME" "ubox" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping pcie.."
"$DUMP_SCRIPT" "$SLOT_NAME" "pcie" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping iio.."
"$DUMP_SCRIPT" "$SLOT_NAME" "iio" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping imc.."
"$DUMP_SCRIPT" "$SLOT_NAME" "imc" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping mesh.."
"$DUMP_SCRIPT" "$SLOT_NAME" "mesh" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping upi.."
"$DUMP_SCRIPT" "$SLOT_NAME" "upi" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping uncore.."
"$DUMP_SCRIPT" "$SLOT_NAME" "uncore" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping tor.."
"$DUMP_SCRIPT" "$SLOT_NAME" "tor" $INTERFACE >> "$CRASHDUMP_FILE"
date
echo "Dumping core mca.."
#MSR dump (core MCA)
"$DUMP_SCRIPT" "$SLOT_NAME" "msr" $INTERFACE >> "$CRASHDUMP_FILE"

# only second/dwr autodump need to rename accordingly
if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
  # dwr
  $DUMP_SCRIPT $SLOT_NAME  "dwr" $INTERFACE >> $CRASHDUMP_FILE

  # rename the archieve file based on whether dump in DWR mode or not
  if [ "$?" == "2" ]; then
    CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_dwr_$SLOT_NAME.tar.gz"
    LOG_MSG_PREFIX="DWR "
  else
    CRASHDUMP_LOG_ARCHIVE="/mnt/data/crashdump_second_$SLOT_NAME.tar.gz"
    LOG_MSG_PREFIX="SECOND_DUMP "
  fi
fi

echo -n "Auto Dump End at " >> $CRASHDUMP_FILE
date >> $CRASHDUMP_FILE

tar zcf $CRASHDUMP_LOG_ARCHIVE -C `dirname $CRASHDUMP_FILE` `basename $CRASHDUMP_FILE` && \
rm -rf $CRASHDUMP_FILE && \
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: $SLOT_NUM is generated at $CRASHDUMP_LOG_ARCHIVE"
cp -f "$CRASHDUMP_LOG_ARCHIVE" /tmp
echo "Auto Dump for $SLOT_NAME Completed"

# Remove current pid file
rm $PID_FILE

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $CRASHDUMP_LOG_ARCHIVE"
exit 0
