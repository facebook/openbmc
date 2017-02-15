#!/bin/bash

ME_UTIL="/usr/local/bin/me-util"

if [ "$#" -eq 1 ] && [ $1 = "time" ]; then
  now=$(date)
  echo "Crash Dump generated at $now"
  exit 0
fi

if [ "$#" -ne 2 ]; then
  echo "$0 <slot#> coreid  ==> for CPU CoreID"
  echo "$0 <slot#> msr     ==> for CPU MSR"
  exit 1
fi

echo ""

if [ "$2" = "coreid" ]; then
  [ -r /usr/local/fbpackages/me-util/crashdump_coreid ] && $ME_UTIL $1 --file /usr/local/fbpackages/me-util/crashdump_coreid
elif [ "$2" = "msr" ]; then
  [ -r /usr/local/fbpackages/me-util/crashdump_msr ] && $ME_UTIL $1 --file /usr/local/fbpackages/me-util/crashdump_msr
fi

exit 0
