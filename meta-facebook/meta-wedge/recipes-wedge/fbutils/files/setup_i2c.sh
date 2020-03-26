#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# Bus 1
i2c_device_add 1 0x60 ncp4200

# Bus 2
i2c_device_add 2 0x60 ncp4200

# Bus 3
i2c_device_add 3 0x48 tmp75
i2c_device_add 3 0x49 tmp75
i2c_device_add 3 0x4a tmp75

# Bus 4
i2c_device_add 4 0x40 fb_panther_plus
i2c_device_add 4 0x51 24c128

# Bus 6, trouble
i2c_device_add 6 0x28 max127
i2c_device_add 6 0x3f pcf8574

# Bus 7
i2c_device_add 7 0x50 24c64         # 6pack power supply EEPROM
i2c_device_add 7 0x51 24c64         # wedge power supply EEPROM
i2c_device_add 7 0x52 24c64         # wedge power supply EEPROM
i2c_device_add 7 0x59 pfe1100       # wedge power supply
i2c_device_add 7 0x5a pfe1100       # wedge power supply

# Bus 8
i2c_device_add 8 0x60 ncp4200

# Bus 12
i2c_device_add 12 0x10 adm1278

i2c_check_driver_binding
