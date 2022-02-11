#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

#
# WDT2C Register. Check AST2500 Datasheet, Chapter 41 "Watchdog Timer"
# for details.
#
WDT2_CTRL_REG=0x1e78502c
WDT2_ENABLE_BIT=0

# Disable the dual boot watchdog
devmem_clear_bit "$WDT2_CTRL_REG" "$WDT2_ENABLE_BIT"

if devmem_test_bit "$WDT2_CTRL_REG" "$WDT2_ENABLE_BIT"; then
    val=$(devmem "$WDT2_CTRL_REG")
    echo "ALERT: failed to disable the 2nd watchdog: WDT2C=$val!!!"
    logger -p user.crit "failed to disable the 2nd watchdog: WDT2C=$val!!!"
    exit 1
fi

echo "Disabled the 2nd watchdog (WDT2) successfully."
