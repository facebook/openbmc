#!/bin/bash

ME_UTIL="/usr/local/bin/me-util"
SENSOR_HISTORY=180

if [ "$#" -eq 1 ] && [ $1 = "time" ]; then
  now=$(date)
  echo "Crash Dump generated at $now"
  exit 0
fi

if [ "$#" -ne 2 ]; then
  echo "$0 <slot#> coreid    ==> for CPU CoreID"
  echo "$0 <slot#> msr       ==> for CPU MSR"
  echo "$0 <slot#> sensors   ==> for sensor history"
  echo "$0 <slot#> threshold ==> for sensor threshold"
  exit 1
fi

echo ""

if [ "$2" = "coreid" ]; then
  [ -r /usr/local/fbpackages/me-util/crashdump_coreid ] && $ME_UTIL $1 --file /usr/local/fbpackages/me-util/crashdump_coreid
elif [ "$2" = "msr" ]; then
  [ -r /usr/local/fbpackages/me-util/crashdump_msr ] && $ME_UTIL $1 --file /usr/local/fbpackages/me-util/crashdump_msr
elif [ "$2" = "sensors" ]; then
  /usr/local/bin/sensor-util $1 --history $SENSOR_HISTORY \
   && /usr/local/bin/sensor-util spb --history $SENSOR_HISTORY \
   && /usr/local/bin/sensor-util nic --history $SENSOR_HISTORY
elif [ "$2" = "threshold" ]; then
  /usr/local/bin/sensor-util all --threshold 
fi

exit 0
