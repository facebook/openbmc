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
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin
# Vcc_Max please refer to Voltage Regulator Module (VRM) and Enterprise
# Voltage Regulator-Down (EVRD) 11.1 Design Guidelines Table 5-4. VR 11.1 
# Voltage Identification (VID) Table ? Part 1 of 2.
# HEX2DEC(VID_VCC_MAX)*2^(-12) = Vcc_Max
VID_VCC_MAX=(0xE66 0xE4D 0xE33 0xE1A 0xE00 0xDE6 0xDCD 0xDB3 0xD9A 0xD80
             0xD66 0xD4D 0xD33 0xD1A 0xD00 0xCE6 0xCCD 0xCB3 0xC9A 0xC80
             0xC66 0xC4D 0xC33 0xC1A 0xC00 0xBE6 0xBCD 0xBB3 0xB9A 0xB80)
XDPE_VOUT_PMBUS_COMMAND=0x21

TH4_ROV=$(i2cget -f -y 12 0x3e 0x56 | awk '{printf "%d", $1}') # Get TH4 AVS value from SYSCPLD

if [[ $TH4_ROV -lt 114 || $TH4_ROV -gt 143 ]]; then 
    echo "Invalid AVS value [0x72 - 0x90]."
    exit 1
fi

TH4_INDEX=$((TH4_ROV - 114))
# Set VOUT to XDPE132G5C
i2cset -f -y 1 0x40 0x00 0xff # Allows access all pages 
XDPE_VDD_MAX_VALUE=$((VID_VCC_MAX[TH4_INDEX]))
XDPE_VDD_MAX_VALUE_HEX=$(printf "0x%04x" $XDPE_VDD_MAX_VALUE) # Convert to HEX

if ! i2cset -f -y 1 0x40 $XDPE_VOUT_PMBUS_COMMAND "$XDPE_VDD_MAX_VALUE_HEX" w; then 
    echo "Cannot communicate with XDPE132G5C."
    exit 1
fi

echo "Setup AVS Success"

