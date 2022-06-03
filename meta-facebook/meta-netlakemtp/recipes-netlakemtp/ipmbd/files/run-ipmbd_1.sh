#!/bin/sh
# first byte: ME i2c-address is 0x10, second byte: 0x10 as BMC's slave address when get response
echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-1/new_device
exec /usr/local/bin/ipmbd 1 1

