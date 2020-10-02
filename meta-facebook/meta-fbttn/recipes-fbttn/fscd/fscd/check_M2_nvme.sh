#!/bin/sh
#This script is for checking if M.2 supports NVMe, by looking for 0x6a address on i2c bus

sleep 30

power_status=`/usr/local/bin/power-util server status | grep ON`

if [ ! -z "$power_status" ]; then
	M2_1_i2c_NVMe=`/usr/sbin/i2cdetect -y 7 | grep 6a`
	M2_2_i2c_NVMe=`/usr/sbin/i2cdetect -y 8 | grep 6a`

	if [ ! -z "$M2_1_i2c_NVMe" ]; then
      kv set M2_1_NVMe 1
	else
      kv set M2_1_NVMe 0
	fi

	if [ ! -z "$M2_2_i2c_NVMe" ]; then
      kv set M2_2_NVMe 1
	else
      kv set M2_2_NVMe 0
	fi
fi
