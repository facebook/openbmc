#!/bin/bash
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
# FMC64 Register. Check AST2600 Datasheet, Chapter 14 "Firmware SPI Memory
# Controller (BSPI)" for details.
#
FMC_WDT2_REG=0x1e620064
FMC_WDT2_ENABLE_BIT=0

# Disable the fmc dual boot watch dog
devmem_clear_bit "$FMC_WDT2_REG" "$FMC_WDT2_ENABLE_BIT"

if devmem_test_bit 0x1e620064 "$FMC_WDT2_ENABLE_BIT"; then
    val=$(devmem "$FMC_WDT2_REG")
    echo "Error: unable to disable the 2nd watchdog: FMC_WDT2=$val"
    exit 1
fi

echo "Disabled the 2nd watchdog (FMC_WDT2) successfully."
