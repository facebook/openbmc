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
set -euo pipefail

readonly DS4520_BUS=8
readonly DS4520_ADDR=0x52

i2cget -f -y "$DS4520_BUS" "$DS4520_ADDR" 0x0 >/dev/null

i2cset -f -y "$DS4520_BUS" "$DS4520_ADDR" 0xf4 0x0
sleep 0.1
i2cset -f -y "$DS4520_BUS" "$DS4520_ADDR" 0xf0 0x40
sleep 0.1
i2cset -f -y "$DS4520_BUS" "$DS4520_ADDR" 0xf1 0x1
sleep 0.1
i2cset -f -y "$DS4520_BUS" "$DS4520_ADDR" 0xf2 0x40
sleep 0.1
i2cset -f -y "$DS4520_BUS" "$DS4520_ADDR" 0xf3 0x1
sleep 0.1
/usr/local/bin/wedge_power.sh reset -s
