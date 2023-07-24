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
. /usr/local/bin/openbmc-utils.sh

# FRU EEPROM
i2c_device_add 3 0x56 24c64    # SCM FRU EEPROM#2
i2c_device_add 6 0x53 24c64    # FCB_B EEPROM
i2c_device_add 8 0x51 24c64    # BMC EEPROM

# FPGA / CPLD
# to access scmcpld need to enable I2C buffer by setting BMC_I2C2_EN to high
gpiocli -s BMC_I2C2_EN set-value 1
sleep 1
i2c_device_add 1 0x35 scmcpld
i2c_device_add 12 0x60 mcbcpld

# CPU IPMI
ulimit -q 1024000
i2c_device_add 5 0x1010 slave-mqueue  # mqueue for ipmi from COMe CPU