#!/bin/sh
. /usr/local/fbpackages/utils/ast-functions
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-0/new_device
exec /usr/local/bin/ipmbd 0 1
