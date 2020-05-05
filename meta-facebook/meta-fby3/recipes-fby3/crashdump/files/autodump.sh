#!/bin/bash
. /usr/local/fbpackages/utils/ast-functions

BOARD_ID=$(get_bmc_board_id)
ME_UTIL="/usr/local/bin/me-util"
FRUID_UTIL="/usr/local/bin/fruid-util"
FW_UTIL="/usr/bin/fw-util"
SLOT_NAME=$1
INTERFACE="PECI_INTERFACE"
PCIE_INTERFACE="PECI_INTERFACE"

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
      if [ $BOARD_ID -eq 9 ]; then
        echo "Usage: $N {slot1}"
      else
        echo "Usage: $N {slot1|slot2|slot3|slot4}"
      fi
      exit 1
      ;;
esac


if [ $BOARD_ID -eq 9 ] && [ $SLOT_NAME != "slot1" ]; then
  # class2 checking
  echo "$SLOT_NAME not supported"
  exit 1
fi

# File format autodump<slot_id>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID=$$
PID_FILE="/var/run/autodump$SLOT_NUM.pid"

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=`cat $PID_FILE` || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# kill previous autodump if exist
if [ ! -z "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null) ; then
  # Check if 2nd dump is running
  if [ ! -z "$(ps | grep '{autodump.sh}' | grep "${SLOT_NAME}" | grep "second\|dwr")" ]; then
    echo $OLDPID > $PID_FILE
    echo "2nd DUMP or Demoted Warm Reset DUMP is running. exit."
    exit 1
  fi

  LOG_FILE="/tmp/autodump.log"
  LOG_ARCHIVE="/mnt/data/autodump_uncompleted_${SLOT_NAME}.tar.gz"
  echo -n "(uncompleted) Auto Dump End at " >> $LOG_FILE
  date >> $LOG_FILE

  tar zcf $LOG_ARCHIVE -C `dirname $LOG_FILE` `basename $LOG_FILE` && \
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
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+1200)) > /tmp/cache_store/fru${SLOT_NUM}_crashdump

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
strings /dev/mtd0 | grep 2019.01 >> $CRASHDUMP_FILE
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
RET=$?

# if ME has response and in operational mode, PECI through ME
if [ "$RET" -eq "0" ] && [ "${RES:6:1}" == "0" ]; then
  RES=$($ME_UTIL $SLOT_NAME 0xb8 0x40 0x57 0x01 0x00 0x30 0x05 0x05 0xa1 0x00 0x00 0x00 0x00)
  RET=$?
  if [ "$RET" -eq "0" ] && [ "${RES:0:11}" == "57 01 00 40" ]; then
    INTERFACE="ME_INTERFACE"
  else
    INTERFACE="PECI_INTERFACE"
  fi
  # else use wired PECI directly
else
  # echo "Use BIC wired PECI interface due to ME abnormal"
  INTERFACE="PECI_INTERFACE"
fi
PCIE_INTERFACE=$INTERFACE

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

  # PCI config read is not support ME DMI interface
  RES=$($ME_UTIL $SLOT_NAME 0xb8 0x40 0x57 0x01 0x00 0x30 0x06 0x05 0x61 0x00 0x00 0x81 0x0D 0x00)
  RET=$?
  if [ "$RET" -eq "0" ] && [ "${RES:0:19}" == "Completion Code: AC" ]; then
    PCIE_INTERFACE="PECI_INTERFACE"
  fi
fi

echo "Set coreid msr interface = $INTERFACE" >> $CRASHDUMP_FILE
echo "Set pcie dwr interface = $PCIE_INTERFACE" >> $CRASHDUMP_FILE

# Sensors & sensor thresholds
echo "Sensor history at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "sensors" >> $CRASHDUMP_FILE
echo "Sensor threshold at dump:" >> $CRASHDUMP_FILE 2>&1
$DUMP_SCRIPT $SLOT_NAME "threshold" >> $CRASHDUMP_FILE

#COREID dump
$DUMP_SCRIPT $SLOT_NAME "coreid" $INTERFACE >> $CRASHDUMP_FILE
#MSR dump
$DUMP_SCRIPT $SLOT_NAME "msr" $INTERFACE >> $CRASHDUMP_FILE
#PCIe dump
$DUMP_SCRIPT $SLOT_NAME "pcie" $PCIE_INTERFACE >> $CRASHDUMP_FILE

# only second/dwr autodump need to rename accordingly
if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
  # dwr
  $DUMP_SCRIPT $SLOT_NAME  "dwr" $PCIE_INTERFACE >> $CRASHDUMP_FILE

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
