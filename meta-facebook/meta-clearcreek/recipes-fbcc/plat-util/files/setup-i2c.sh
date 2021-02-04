#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

KVSET_CMD=/usr/bin/kv

echo "Setup TMP422"
i2c_device_add 6 0x4d tmp422
i2c_device_add 6 0x4e tmp422

echo "Setup TCA9539 input/output pin"
i2cset -y -f 5 0x77 0x06 0x00
i2cset -y -f 5 0x77 0x07 0x0f
i2cset -y -f 5 0x77 0x02 0x0f

echo "Setup VR sensor"
i2c_device_add 5 0x30 mpq8645p
i2c_device_add 5 0x31 mpq8645p
i2c_device_add 5 0x32 mpq8645p
i2c_device_add 5 0x33 mpq8645p
i2c_device_add 5 0x34 mpq8645p
i2c_device_add 5 0x35 mpq8645p
i2c_device_add 5 0x36 mpq8645p
i2c_device_add 5 0x3b mpq8645p

echo "Setup TMP75"
i2c_device_add 21 0x48 tmp75
i2c_device_add 21 0x49 tmp75
i2c_device_add 22 0x48 tmp75
i2c_device_add 22 0x49 tmp75

#Set PWR_AVR to 128 samples
i2cset -y -f 5 0x11 0xd4 0x3F1E w

#Set FIO LED
i2cset -y -f 0x08 0x77 0x7 0x3F
i2cset -y -f 0x08 0x77 0x3 0x80

#Set M.2 carrier INA219 Calibration register
i2cset -y -f 21 0x45 5 0xC800 w
i2cset -y -f 22 0x45 5 0xC800 w

#Check carrier#0 type
ipmitool raw 0x6 0x52 0x7 0xe0 0 01 0x80
carrier0=`ipmitool raw 0x6 0x52 0x7 0x2e 0x1 0x0 0x5`
if [ $carrier0 = "cc" ]; then
    $KVSET_CMD set "carrier_0" "m.2"
    i2cset -y -f 21 0x77 0x2 0xFF
elif [ $carrier0 = "44" ]; then
    $KVSET_CMD set "carrier_0" "e1.s"
    i2cset -y -f 21 0x77 0x02 0x00
fi

#Check carrier#1 type
ipmitool raw 0x6 0x52 0x7 0xe0 0 01 0x80
carrier1=`ipmitool raw 0x6 0x52 0x7 0x2e 0x1 0x0 0x6`
if [ $carrier1 = "cc" ]; then
    $KVSET_CMD set "carrier_1" "m.2"
    i2cset -y -f 22 0x77 0x2 0xFF
elif [ $carrier1 = "44" ]; then
    $KVSET_CMD set "carrier_1" "e1.s"
    i2cset -y -f 22 0x77 0x02 0x00
fi

# Mark driver loaded
echo > /tmp/driver_probed
