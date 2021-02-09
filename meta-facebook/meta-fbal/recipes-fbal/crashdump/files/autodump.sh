#!/bin/bash

PID=$$
# File format autodump<fru>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE='/var/run/autodump1.pid'
DUMP_DIR='/tmp/crashdumps'
PARSE_FILE="$DUMP_DIR/bafi.log"
DUMP_FILE="$DUMP_DIR/sensordump.log"
DUMP_UTIL='/usr/bin/acd'
PARSE_UTIL='/usr/bin/acd-analyzer'
LOG_ARCHIVE='/mnt/data/autodump.tar.gz'
ACD=true

if [ ! -f $DUMP_UTIL ] ; then
  . /usr/local/bin/gpio-utils.sh

  ACD=false
  DUMP_FILE="$DUMP_DIR/autodump.log"
  DUMP_UTIL='/usr/local/bin/dump.sh'
  mkdir -p $DUMP_DIR
fi

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=$(cat $PID_FILE) || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# Set crashdump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
kv set fru1_crashdump $((sys_runtime+630))

# kill previous autodump if exist
if [ ! -z "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null) ; then
  if [ "$ACD" != true ] ; then
    LOG_ARCHIVE='/mnt/data/autodump_uncompleted.tar.gz'
    echo -n "(uncompleted) Auto Dump End at " >> $DUMP_FILE
    date >> $DUMP_FILE
    tar zcf $LOG_ARCHIVE -C $DUMP_DIR `basename $DUMP_FILE`
  fi

  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  killall -s 9 peci-util >/dev/null 2>&1
  killall -s 9 acd >/dev/null 2>&1
  killall -s 9 acd-analyzer >/dev/null 2>&1
fi
unset OLDPID

DELAY_SEC=30
SENSOR_HISTORY=180

while test $# -gt 0
do
  case "$1" in
  --now)
    DELAY_SEC=0
    ;;
  *)
    echo "unknown argument $1"
    ;;
  esac
  shift
done

if [ "$DELAY_SEC" != "0" ]; then
  echo "Auto Dump will start after ${DELAY_SEC}s..."
  sleep ${DELAY_SEC}
fi

# Block Power Cycle
/usr/local/bin/ipmb-util 8 0x68 0xd8 0xf2 0x00 0x07 0x01
echo "Auto Dump Started"

LOGS=$(basename $DUMP_FILE)
if [ "$ACD" == true ] ; then
  $DUMP_UTIL

  LOG_FILE=$(ls /tmp/crashdumps/ -t1 |grep ^crashdump_ |head -n 1)
  $PARSE_UTIL "$DUMP_DIR/$LOG_FILE" > $PARSE_FILE 2>&1

  LOGS="$LOGS $LOG_FILE $(basename $PARSE_FILE)"
  echo -n "Sensor Dump Start at " > $DUMP_FILE
  date >> $DUMP_FILE

else

  echo -n "Auto Dump Start at " > $DUMP_FILE
  date >> $DUMP_FILE

  # CPU0
  $DUMP_UTIL 48 coreid >> $DUMP_FILE 2>&1
  $DUMP_UTIL 48 msr >> $DUMP_FILE 2>&1

  # CPU1
  if [ "$(gpio_get_value FM_CPU1_SKTOCC_LVT3_PLD_N)" -eq "0" ]; then
    $DUMP_UTIL 49 coreid >> $DUMP_FILE 2>&1
    $DUMP_UTIL 49 msr >> $DUMP_FILE 2>&1
  else
    echo "<<---------- CPU1 is not present, skip it. ---------->>" >> $DUMP_FILE
  fi

  # PCIe
  #$DUMP_UTIL pcie >> $DUMP_FILE 2>&1
fi

# Sensors
echo "Sensor history of last ${SENSOR_HISTORY}s at dump:" >> $DUMP_FILE 2>&1
/usr/local/bin/sensor-util all --history $SENSOR_HISTORY >> $DUMP_FILE 2>&1

echo "Sensor threshold at dump: " >> $DUMP_FILE 2>&1
/usr/local/bin/sensor-util all --threshold >> $DUMP_FILE 2>&1

echo -n "Dump End at " >> $DUMP_FILE
date >> $DUMP_FILE

tar zcf $LOG_ARCHIVE -C $DUMP_DIR $LOGS
logger -t "ipmid" -p daemon.crit "Crashdump for FRU: 1 is generated at $LOG_ARCHIVE"

# Remove current pid file
rm $PID_FILE

# Unblock Power Cycle
/usr/local/bin/ipmb-util 8 0x68 0xd8 0xf2 0x00 0x07 0x00
echo "Auto Dump Stored in $LOG_ARCHIVE"
exit 0
