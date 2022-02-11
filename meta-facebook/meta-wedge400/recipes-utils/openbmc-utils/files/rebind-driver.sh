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
# the root cause has not been found. When the issue happened, we measured the voltage
# and it was normal, also reload the pmbus device driver will fix the issue.

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

printf "Check special sensors and rebind driver..."

retry=0
while [ $retry -lt 5 ]; do
    sensor_ok=$(/usr/local/bin/sensor-util smb 0x55 --force | grep ok &> /dev/null; echo $?)
    if [ $((sensor_ok)) -eq 0 ]; then
        echo "Done"
        exit 0
    fi
    retry=$((retry + 1))
    echo "trying rebind driver pxe1211 for i2c device 1-0e, time $retry"
    i2c_device_delete 1 0x0e
    i2c_device_add 1 0x0e pxe1211
done
