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

# Bus 0
i2c_device_add 0 0x10 adm1278
i2c_device_add 0 0x18 ds110df111
i2c_device_add 0 0x21 pca9534
i2c_device_add 0 0x33 galaxy100_ec
i2c_device_add 0 0x3e scmcpld
i2c_device_add 0 0x54 24c02
i2c_device_add 0 0x52 24c02

# Bus 1
i2c_device_add 1 0x18 ds110df111
i2c_device_add 1 0x70 IR3581
i2c_device_add 1 0x72 IR3584

# Bus 2
i2c_device_add 2 0x40 pwr1014a

# Bus 5
i2c_device_add 5 0x20 slb9645tt

# Bus 6
i2c_device_add 6 0x27 pcf8574
i2c_device_add 6 0x51 24c64

# Bus 9
i2c_device_add 9 0x50 24c02

# Bus 11
i2c_device_add 11 0x10 adm1278

# Bus 12
i2c_device_add 12 0x31 syscpld
i2c_device_add 12 0x39 qsfp_cpld

#
# setup ADC channels
# This step is only needed in legacy kernel (v4.1), because the ADC
# controller is configured in device tree in the latest kernel.
#
if uname -r | grep "4\.1\.*" > /dev/null 2>&1; then
    modprobe ast_adc
    # channel 3: r1:  2.0K; r2:  1.0K; v2: 0mv
    # channel 4: r1:  4.7K; r2:  1.0K; v2: 0mv
    # channel 8: r1:  1.0K; r2:  1.0K; v2: 0mv
    # channel 9: r1:  1.0K; r2:  1.0K; v2: 0mv
    ast_config_adc 3  2 1 0
    ast_config_adc 4  47 10 0
    ast_config_adc 8  1 1 0
    ast_config_adc 9  1 1 0
fi
