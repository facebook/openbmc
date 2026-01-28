#!/bin/bash

# In CPU mode, the console is 9600, so update it prior to switching.
aconf_util.sh program cpu

set -x
i2cset -f -y 9 0x11 0xfa 0x11
sleep 0.1
i2cset -f -y 9 0x11 0xfb 0x7
sleep 0.1
i2cset -f -y 9 0x11 0x11
sleep 0.1
/usr/local/bin/wedge_power.sh reset -s
