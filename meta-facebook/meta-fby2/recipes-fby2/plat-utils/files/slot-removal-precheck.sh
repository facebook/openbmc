#!/bin/bash

SLOT=$1

PWR_UTIL_PID=$(ps | grep power-util | grep $SLOT | awk '{print $1}')
HSV_REINIT_PID=$(ps | grep hotservice-reinit.sh | grep $SLOT | awk '{print $1}')

if [ ! -z $PWR_UTIL_PID ]; then
  logger -p user.info "slot-removal-precheck.sh: kill power-util $SLOT, pid: $PWR_UTIL_PID"
  kill -9 $PWR_UTIL_PID
fi

if [ ! -z $HSV_REINIT_PID ]; then
  logger -p user.info "slot-removal-precheck.sh: kill hotservice-reinit.sh $SLOT, pid: $HSV_REINIT_PID"
  kill -9 $HSV_REINIT_PID
fi

