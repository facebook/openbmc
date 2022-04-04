#!/bin/bash
#
# Copyright (c) 2022 Cisco Systems Inc.
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
. /usr/local/bin/openbmc-utils.sh

#BMC Temp sensor
i2c_device_add 8 0x4A lm75

#BMC IDPROM
i2c_device_add 8 0x54 24c64

#Chassis IDPROM
i2c_device_add 8 0x56 24c64

#enable access to GE switch EEPROM
i2cset -f -y 12 0x31 0x0c
i2cset -f -y 12 0x32 0x00
i2cset -f -y 12 0x33 0x00
i2cset -f -y 12 0x34 0x00
i2cset -f -y 12 0x35 0x07
i2cset -f -y 12 0x36 0x2a
i2cset -f -y 12 0x30 0x0

#GE switch EEPROM
i2c_device_add 8 0x50 24c04

#disable access to GE switch EEPROM
i2cset -f -y 12 0x31 0x0c
i2cset -f -y 12 0x32 0x00
i2cset -f -y 12 0x33 0x00
i2cset -f -y 12 0x34 0x00
i2cset -f -y 12 0x35 0x03
i2cset -f -y 12 0x36 0x2a
i2cset -f -y 12 0x30 0x0

