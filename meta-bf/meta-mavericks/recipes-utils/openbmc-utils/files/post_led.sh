#!/bin/sh
#
# Copyright 2004-present Facebook. All rights reserved.
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

usage() {
    echo "Displays values onto the debug header LEDs."
    echo "Hex and decimal accepted."
    echo "Usage: $0 <value>"
}

. /usr/local/bin/openbmc-utils.sh

display_value() {
    num=0
    input=$1
    while [ $num -lt 8 ]; do  {
        gpio_set $(gpio_name2value LED_POSTCODE_$num) $((input & 1))
        input=$((input >> 1))
        num=$((num + 1))
    }
    done
}

# Check number of parameters
if [ $# -ne 1 ]
then
    usage
    exit 1
fi

# Make sure input is actually numeric
DEC_VALUE=$(printf "%d" $1 2>/dev/null)
if [ $? -eq 1 ]
then
    echo "Unable to parse input as numeric value."
    exit 1
fi

# Make sure input is within proper range
if [ $DEC_VALUE -lt 0 ] || [ $DEC_VALUE -gt 255 ]
then
    echo "Value $DEC_VALUE is outside of displayable range 0 - 0xff (255)."
    exit 1
fi

display_value $DEC_VALUE
