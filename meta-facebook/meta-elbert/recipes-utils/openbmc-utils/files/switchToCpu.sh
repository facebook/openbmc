#!/bin/bash

# In CPU mode, the console is 9600, so update it prior to switching.
aconf_util.sh program cpu

set -x
i2cset -f -y 8 0x50 0xf4 0x0
sleep 0.1
i2cset -f -y 8 0x50 0xf0 0xa5
sleep 0.1
i2cset -f -y 8 0x50 0xf1 0x1
sleep 0.1
i2cset -f -y 8 0x50 0xf2 0xa5
sleep 0.1
i2cset -f -y 8 0x50 0xf3 0x1
sleep 0.1
/usr/local/bin/wedge_power.sh reset -s
