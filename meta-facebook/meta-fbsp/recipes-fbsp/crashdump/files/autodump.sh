#!/bin/bash

PID=$$
# File format autodump<fru>.pid (See pal_is_crashdump_ongoing()
# function definition)
PID_FILE='/var/run/autodump1.pid'
DWR_FILE='/tmp/DWR'

. /usr/local/bin/gpio-utils.sh

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=`cat $PID_FILE` || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# Set crashdump timestamp
sys_runtime=$(awk '{print $1}' /proc/uptime)
sys_runtime=$(printf "%0.f" $sys_runtime)
echo $((sys_runtime+630)) > /tmp/cache_store/fru1_crashdump

# kill previous autodump if exist
if [ ! -z "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null) ; then
  LOG_FILE='/tmp/autodump.log'
  LOG_ARCHIVE='/mnt/data/autodump_uncompleted.tar.gz'
  echo -n "(uncompleted) Auto Dump End at " >> $LOG_FILE
  date >> $LOG_FILE

  tar zcf $LOG_ARCHIVE -C `dirname $LOG_FILE` `basename $LOG_FILE` && \
  rm -rf $LOG_FILE && \

  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  killall peci-util &> /dev/null
fi
unset OLDPID

DUMP_SCRIPT='/usr/local/bin/dump.sh'
LOG_FILE='/tmp/autodump.log'
LOG_ARCHIVE='/mnt/data/autodump.tar.gz'
LOG_MSG_PREFIX=""

CPU1_SKTOCC_N="FM_CPU1_SKTOCC_LVT3_PLD_N"
DWR=0
SECOND_DUMP=0
DELAY_SEC=30
SENSOR_HISTORY=180

while test $# -gt 0
do
  case "$1" in
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
    echo "unknown argument $1"
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

echo "Auto Dump Started"

# Start to dump and clear LOG_FILE
echo -n "Auto Dump Start at " > $LOG_FILE
date >> $LOG_FILE

# CPU0
$DUMP_SCRIPT 48 coreid >> $LOG_FILE 2>&1
$DUMP_SCRIPT 48 msr >> $LOG_FILE 2>&1

# CPU1
if [ "$(gpio_get_value ${CPU1_SKTOCC_N})" -eq "0" ]; then
  $DUMP_SCRIPT 49 coreid >> $LOG_FILE 2>&1
  $DUMP_SCRIPT 49 msr >> $LOG_FILE 2>&1
else
  echo "<<---------- CPU1 is not present, skip it. ---------->>" >> $LOG_FILE
fi

# PCIe
#$DUMP_SCRIPT pcie >> $LOG_FILE 2>&1

# Sensors
echo "Sensor history of last ${SENSOR_HISTORY}s at dump:" >> $LOG_FILE 2>&1
/usr/local/bin/sensor-util all --history $SENSOR_HISTORY >> $LOG_FILE 2>&1

echo "Sensor threshold at dump: " >> $LOG_FILE 2>&1
/usr/local/bin/sensor-util all --threshold >> $LOG_FILE 2>&1

# only second/dwr autodump need to rename accordingly
if [ "$DWR" == "1" ] || [ "$SECOND_DUMP" == "1" ]; then
# dwr
$DUMP_SCRIPT dwr >> $LOG_FILE 2>&1

# rename the archieve file based on whether dump in DWR mode or not
if [ "$?" == "2" ]; then
  LOG_ARCHIVE='/mnt/data/autodump_dwr.tar.gz'
  LOG_MSG_PREFIX="DWR "
else
  LOG_ARCHIVE='/mnt/data/autodump_second.tar.gz'
  LOG_MSG_PREFIX="SECOND_DUMP "
fi
fi

echo -n "Auto Dump End at " >> $LOG_FILE
date >> $LOG_FILE

tar zcf $LOG_ARCHIVE -C `dirname $LOG_FILE` `basename $LOG_FILE` && \
rm -rf $LOG_FILE && \
logger -t "ipmid" -p daemon.crit "${LOG_MSG_PREFIX}Crashdump for FRU: 1 is generated at $LOG_ARCHIVE"

# Remove current pid file
rm $PID_FILE
rm -r $DWR_FILE >/dev/null 2>&1

echo "${LOG_MSG_PREFIX}Auto Dump Stored in $LOG_ARCHIVE"
exit 0
