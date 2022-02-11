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

# Bus 3
i2c_device_add 3 0x48 tmp75
i2c_device_add 3 0x49 tmp75
i2c_device_add 3 0x4a tmp75
i2c_device_add 3 0x4b tmp75
i2c_device_add 3 0x4c tmp75

# Bus 4
i2c_device_add 4 0x33 com_e_driver

# Bus 6
i2c_device_add 6 0x2f pcf8574
i2c_device_add 6 0x51 24c64

# Bus 7
i2c_device_add 7 0x50 24c02         # BMC PHY EEPROM
i2c_device_add 7 0x51 24c64         # PFE1100 power supply EEPROM
i2c_device_add 7 0x52 24c64         # PFE1100 power supply EEPROM
i2c_device_add 7 0x6f ltc4151

# Bus 8
i2c_device_add 8 0x33 fancpld
i2c_device_add 8 0x48 tmp75
i2c_device_add 8 0x49 tmp75

# Bus 9
# 9 0x20 tpm_i2c_infineon added by trousers.init.sh

# Bus 12
i2c_device_add 12 0x31 syscpld
