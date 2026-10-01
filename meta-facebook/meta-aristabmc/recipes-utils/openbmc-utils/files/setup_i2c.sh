#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

# Board EEPROMs (AT24-compatible 512-Kbit devices).
i2c_device_add 14 0x50 24c512 # BMC EEPROM
i2c_device_add 9 0x50 24c512  # CPU EEPROM
i2c_device_add 9 0x52 24c512  # SMB EEPROM
i2c_device_add 9 0x53 24c512  # Chassis EEPROM

# Instantiate the CPU power CPLD.
i2c_device_add 12 0x43 pwrcpld
modprobe pwrcpld

# Instantiate the switch-card/management-card CPLD on BMC SMBus 9.
i2c_device_add 9 0x23 smbcpld
