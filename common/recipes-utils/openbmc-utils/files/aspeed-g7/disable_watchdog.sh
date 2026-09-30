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

# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

# WDT0C Register. Check AST2750 A2 Datasheet, Chapter 62 "Watchdog Timer (WDT)"
# for details.
# 
WDTA_CONTROL_REG=0x14c3740c
WDTA_ENABLE_BIT=0

# Disable the 2nd watchdog
devmem_clear_bit "$WDTA_CONTROL_REG" "$WDTA_ENABLE_BIT"

if devmem_test_bit "$WDTA_CONTROL_REG" "$WDTA_ENABLE_BIT"; then
    val=$(devmem "$WDTA_CONTROL_REG")
    echo "Error: unable to disable the 2nd watchdog: WDTA=$val"
    exit 1
fi

echo "Disabled the 2nd watchdog (WDTA) successfully."
