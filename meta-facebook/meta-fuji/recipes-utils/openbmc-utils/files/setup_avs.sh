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

# Check board version. Only set avs VOUT from EVT3
# Since the TH4 AVS feature can't be enabled on EVT1,2 hardware units because of hardware factors. 
# But we found the SMB EVT1 board ID is incorrect, it's the same as DVT1 board ID, 
# and there are 3 EVT1 units(F301220170001, F301220170002, F301220240001) shipped to FB. 
# So it seems FB can't upgrade the 3 EVT1 units to next package.

BOARD_VER=$(i2cget -f -y 13 0x35 0x3 | awk '{printf "%d", $1}') #Get board version
if [ "$BOARD_VER" -lt 66 ];then
    echo "No need to set avs VOUT."
    exit 1
fi

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

VID_VCC_MAX=(0xE6B 0xE0C 0xDB2 0xD58 0xCF6)
XDPE_VOUT_PMBUS_COMMAND=0x21

TH4_ROV=$(i2cget -f -y 12 0x3e 0x56 | awk '{printf "%d", $1}') # Get TH4 AVS value from SYSCPLD

case $TH4_ROV in
    126|130|134|138|142) #0x7e,0x82,0x86,0x8a,0x8e
        ;;
    *)
        echo "Invalid AVS value[126,130,134,138,142]"
        exit 1
        ;;
esac

TH4_INDEX=$(((TH4_ROV - 126) / 4))
# Set VOUT to XDPE132G5C
i2cset -f -y 1 0x40 0x00 0xff # Allows access all pages 
XDPE_VDD_MAX_VALUE=$((VID_VCC_MAX[TH4_INDEX]))
XDPE_VDD_MAX_VALUE_HEX=$(printf "0x%04x" $XDPE_VDD_MAX_VALUE) # Convert to HEX

if ! i2cset -f -y 1 0x40 $XDPE_VOUT_PMBUS_COMMAND "$XDPE_VDD_MAX_VALUE_HEX" w; then 
    echo "Cannot communicate with XDPE132G5C."
    exit 1
fi

echo "Setup AVS Success"

