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

# Function to set the less significant hex digit
display_lower() {  
    local bit0=$(expr $1 % 2)
    local bit1=$(expr $1 / 2 % 2)
    local bit2=$(expr $1 / 4 % 2)
    local bit3=$(expr $1 / 8 % 2)
    
    # Now set the GPIOs to the right binary values
    gpio_set_value LED_POSTCODE_0 "$bit0"
    gpio_set_value LED_POSTCODE_1 "$bit1"
    gpio_set_value LED_POSTCODE_2 "$bit2"
    gpio_set_value LED_POSTCODE_3 "$bit3"
}

# Function to set the more significant hex digit
display_upper() {
    local bit0=$(expr $1 % 2)
    local bit1=$(expr $1 / 2 % 2)
    local bit2=$(expr $1 / 4 % 2)
    local bit3=$(expr $1 / 8 % 2)

    gpio_set_value LED_POSTCODE_4 "$bit0"
    gpio_set_value LED_POSTCODE_5 "$bit1"
    gpio_set_value LED_POSTCODE_6 "$bit2"
    gpio_set_value LED_POSTCODE_7 "$bit3"
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

# Get upper/lower decimal values
LOWER_DEC_VALUE=$(expr $DEC_VALUE % 16)
UPPER_DEC_VALUE=$(expr $DEC_VALUE / 16)

# Display the results
display_lower $LOWER_DEC_VALUE
display_upper $UPPER_DEC_VALUE
