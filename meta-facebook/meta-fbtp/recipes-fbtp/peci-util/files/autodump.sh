#!/bin/bash

PID=$$
PID_FILE='/var/run/autodump.pid'

# check if running auto dump
[ -r $PID_FILE ] && OLDPID=`cat $PID_FILE` || OLDPID=''

# Set current pid
echo $PID > $PID_FILE

# kill previous autodump if exist
if [ ! -z "$OLDPID" ] && (grep "autodump" /proc/$OLDPID/cmdline &> /dev/null) ; then
  echo "kill pid $OLDPID..."
  kill -s 9 $OLDPID
  killall peci-util
fi
unset OLDPID


PECI_UTIL='/usr/local/bin/peci-util'
LOG_FILE='/tmp/autodump.log'
LOG_ARCHIVE='/mnt/data/autodump.tar.gz'

CPU0_SKTOCC_N="/sys/class/gpio/gpio51/value"
CPU1_SKTOCC_N="/sys/class/gpio/gpio208/value"
DELAY_SEC=30

echo "Auto Dump will start after ${DELAY_SEC}s..."

sleep ${DELAY_SEC}

echo "Auto Dump Started"

# Start temp and clear LOG_FILE
echo -n "Auto Dump Start at " > $LOG_FILE
date >> $LOG_FILE
# CPU0
[ -r /etc/peci/crashdump_p0_coreid ] && \
  $PECI_UTIL --input /etc/peci/crashdump_p0_coreid >> $LOG_FILE 2>&1
[ -r /etc/peci/crashdump_p0_msr ] && \
  $PECI_UTIL --input /etc/peci/crashdump_p0_msr >> $LOG_FILE 2>&1
# CPU1
if [ "`cat $CPU1_SKTOCC_N`" -eq "0" ]; then
  [ -r /etc/peci/crashdump_p1_coreid ] && \
    $PECI_UTIL --input /etc/peci/crashdump_p1_coreid >> $LOG_FILE 2>&1
  [ -r /etc/peci/crashdump_p1_msr ] && \
    $PECI_UTIL --input /etc/peci/crashdump_p1_msr >> $LOG_FILE 2>&1
else
  echo "<<---------- CPU1 is not present, skip it. ---------->>" >> $LOG_FILE
fi
# PCIe
[ -r /etc/peci/crashdump_pcie ] && \
  $PECI_UTIL --retry 0 --input /etc/peci/crashdump_pcie >> $LOG_FILE 2>&1

tar zcf $LOG_ARCHIVE -C `dirname $LOG_FILE` `basename $LOG_FILE` && \
rm -rf $LOG_FILE

# Remove current pid file
rm $PID_FILE

echo "Auto Dump Stored in $LOG_ARCHIVE"
