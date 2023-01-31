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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

#
# Create i2c switches at the beginning so all the channels are properly
# initialized when the child devices are created: the step is only needed
# for kernel 4.1 as i2c switches are handled in device tree for newer
# kernel versions.
#
# 7-0070 is the i2c-switch for ltc power supply in DC deployment. It has
# 8 child channels which are assigned i2c bus 14-21. 2 PFE1100 power
# supplies are connected to channel #0 (14-005a) and #1 (15-0059).
#
PSU_MUX_DEV="7-0070"
PSU_MUX_PATH=$(i2c_device_sysfs_abspath "$PSU_MUX_DEV")
if [ ! -d "$PSU_MUX_PATH" ]; then
    i2c_mux_add_sync 7 0x70 pca9548 8
fi

# Bus 2
i2c_device_add 2 0x3a pwr1014a

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

# Bus 8
i2c_device_add 8 0x33 fancpld
i2c_device_add 8 0x48 tmp75
i2c_device_add 8 0x49 tmp75

# Bus 9   TPM driver for TCSD service
i2c_device_add 9 0x20 tpm_i2c_infineon

# Bus 12
i2c_device_add 12 0x31 syscpld

i2c_check_driver_binding
