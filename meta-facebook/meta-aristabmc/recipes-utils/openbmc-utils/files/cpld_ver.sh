#!/bin/bash
#
# Copyright 2026-present Facebook. All Rights Reserved.
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
#
# Read the host CPU CPLD revision through its BMC I2C interface. The register
# map defines minor revision at 0x00 and major revision at 0x01.

set -euo pipefail

readonly CPLD_BUS=12
readonly CPLD_ADDRESS=0x43
readonly CPLD_MINOR_REGISTER=0x00
readonly CPLD_MAJOR_REGISTER=0x01

read_register() {
    i2cget -f -y "$CPLD_BUS" "$CPLD_ADDRESS" "$1"
}

minor="$(read_register "$CPLD_MINOR_REGISTER")"
major="$(read_register "$CPLD_MAJOR_REGISTER")"

printf 'CPU CPLD: %d.%d\n' "$((major))" "$((minor))"
