#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

# This script is a workaround for the I2C PMBUS vout sensor reading issue given
# the root cause has not been found. When the issue happens, reload the pmbus
# device driver will fix the issue.

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

printf "Check special sensors and rebind driver..."

i2c_check_driver_binding "fix-binding"

/usr/local/bin/xdpe12284-hack.sh

retry=0
# ir35215 chip enabled by COMe power on, 
# so need sleep some time to wait COMe Power on
sleep 5
#
# We have seen intermittent ir35215 voltage reading value unreasonable and root cause is
# not clear as of now. Testing shows below retry will help to mitigate
# such error.
#
fixup_i2c_ir35215() {
    while [ $retry -lt 5 ]; do
        sensor1_ok=$(/usr/local/bin/sensor-util smb 0x12 --force | grep ok &> /dev/null; echo $?)
        sensor2_ok=$(/usr/local/bin/sensor-util smb 0x20 --force | grep ok &> /dev/null; echo $?)
        if [ $((sensor1_ok)) -eq 0 ] && [ $((sensor2_ok)) -eq 0 ]; then
            echo "Check ir35215 done"
            return
        fi
        retry=$((retry + 1))
        echo "trying fixup driver ir35215 for i2c device 16-4d, time $retry"
        i2c_device_delete 16 0x4d
        i2c_device_add 16 0x4d ir35215
    done
    logger -p user.err "Unable to fix ir35215 sensor reading after 5 retries"
}
#
# We have seen intermittent pxe1211 voltage reading value unreasonable and root cause is
# not clear as of now. Testing shows below retry will help to mitigate
# such error.
#
retry=0
fixup_i2c_pxe1211() {
    while [ $retry -lt 5 ]; do
        sensor1_ok=$(/usr/local/bin/sensor-util smb 0x6e --force | grep ok &> /dev/null; echo $?)
        sensor2_ok=$(/usr/local/bin/sensor-util smb 0x75 --force | grep ok &> /dev/null; echo $?)
        sensor3_ok=$(/usr/local/bin/sensor-util smb 0x7c --force | grep ok &> /dev/null; echo $?)
        if [ $((sensor1_ok)) -eq 0 ] && [ $((sensor2_ok)) -eq 0 ]  && [ $((sensor3_ok)) -eq 0 ]; then
            echo "Check pxe1211 done"
            return
        fi
        retry=$((retry + 1))
        echo "trying fixup driver pxe1211 for i2c device 20-0e, time $retry"
        i2c_device_delete 20 0x0e
        i2c_device_add 20 0x0e pxe1211
    done
    logger -p user.err "Unable to fix pxe1211 sensor reading after 5 retries"
}
#
# We have seen intermittent xdpe132g5c voltage reading value unreasonable and root cause is
# not clear as of now. Testing shows below retry will help to mitigate
# such error.
#
retry=0
fixup_i2c_xdpe132g5c() {
    while [ $retry -lt 5 ]; do
        sensor1_ok=$(/usr/local/bin/sensor-util smb 0x35 --force | grep ok &> /dev/null; echo $?)
        sensor2_ok=$(/usr/local/bin/sensor-util smb 0x2e --force | grep ok &> /dev/null; echo $?)
        if [ $((sensor1_ok)) -eq 0 ] && [ $((sensor2_ok)) -eq 0 ]; then
            echo "Check xdpe132g5c done"
            return
        fi
        retry=$((retry + 1))
        echo "trying fixup driver xdpe132g5c for i2c device 17-40, time $retry"
        i2c_device_delete 17 0x40
        i2c_device_add 17 0x40 xdpe132g5c
    done
    logger -p user.err "Unable to fix xdpe132g5c sensor reading after 5 retries"
}

fixup_i2c_ir35215
fixup_i2c_pxe1211
fixup_i2c_xdpe132g5c
