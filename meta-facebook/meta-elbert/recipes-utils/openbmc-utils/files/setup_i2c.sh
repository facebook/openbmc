#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

# ELBERTTODO 442091 Probe FSCD devices first in case one module is taking too
# long or is in bad shape

# ELBERTTODO Take MUX out of reset

# SMBus 0
# Currently not populating I2C TPM
# i2c_device_add 0 0x20 tpm_i2c_infineon

# ELBERTTODO 442078 SCDCPLD SUPPORT
# SMBus 4
# i2c_device_add 4 0x23 scdcpld
# i2c_device_add 4 0x50 24c512
# This eeprom contains SC FRU information
# i2c_device_add 4 0x51 24c512

# ELBERTTODO 442083 implement fancpld
# SMBus 6
# i2c_device_add 6 0x60 fancpld

# SMBus 7 CHASSIS EEPROM
i2c_device_add 6 0x52 24c512

# SMBus 9 SUP DPM UCD90320
#i2c_device_add 9 0x11 pmbus

# SMBus 10 SUP POWER
i2c_device_add 10 0x30 cpupwr

# SMBus 11 SUP TEMP, POWER
i2c_device_add 11 0x37 mempwr
i2c_device_add 11 0x4c max6658
i2c_device_add 11 0x40 pmbus

# Bus  12 SUP CPLD, SUP EEPROM
i2c_device_add 12 0x43 supcpld
i2c_device_add 12 0x50 24c512

# SMBus 13
# ELBERTTODO SSD EEPROM ?

# ELBERTTODO 451861 LC SMBus support
# SMBus 2 LC MUX, muxed as Bus 14-21
# Bus  14 - Muxed LC1
# Bus  15 - Muxed LC2
# Bus  16 - Muxed LC3
# Bus  17 - Muxed LC4
# Bus  18 - Muxed LC5
# Bus  19 - Muxed LC6
# Bus  20 - Muxed LC7
# Bus  21 - Muxed LC8

# ELBERTTODO 451859 PSU support - MUXED
# SMBus 5 PSU MUX, muxed as Bus 22-25
# Do we need to do write protect?
# Bus  22 - Muxed PSU1
# Bus  23 - Muxed PSU2
# Bus  24 - Muxed PSU3
# Bus  25 - Muxed PSU4
