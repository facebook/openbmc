#!/bin/bash

# In CPU mode, the console is 9600, so update it prior to switching.
aconf_util.sh program cpu

DEV_ADDR=""
if i2cget -y 8 0x52 0x0 &> /dev/null; then
   DEV_ADDR=0x52
else
   # P1 systems have DS4520 at 0x50
   echo "DS4520 not detected at 8-0052, trying 8-0050"
   DEV_ADDR=0x50
fi

set -x
i2cset -f -y 8 "$DEV_ADDR" 0xf4 0x0
sleep 0.1
i2cset -f -y 8 "$DEV_ADDR" 0xf0 0x20
sleep 0.1
i2cset -f -y 8 "$DEV_ADDR" 0xf1 0x1
sleep 0.1
i2cset -f -y 8 "$DEV_ADDR" 0xf2 0x20
sleep 0.1
i2cset -f -y 8 "$DEV_ADDR" 0xf3 0x1
sleep 0.1
/usr/local/bin/wedge_power.sh reset -s
