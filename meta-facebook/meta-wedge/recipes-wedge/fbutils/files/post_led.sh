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
    
    # Set the pins to the correct operating mode.
    # The relevant pins are GPIOG[0...3].
    # For GPIO bank G, SCU84[0..3] must be 0.
    devmem_clear_bit $(scu_addr 84) 0
    devmem_clear_bit $(scu_addr 84) 1
    devmem_clear_bit $(scu_addr 84) 2
    devmem_clear_bit $(scu_addr 84) 3

    # Now set the GPIOs to the right binary values
    gpio_set 48 $bit0
    gpio_set 49 $bit1
    gpio_set 50 $bit2
    gpio_set 51 $bit3
}

# Function to set the more significant hex digit
display_upper() {
    local bit0=$(expr $1 % 2)
    local bit1=$(expr $1 / 2 % 2)
    local bit2=$(expr $1 / 4 % 2)
    local bit3=$(expr $1 / 8 % 2)

    # Set the pins to the correct operating mode.
    # The relevant pins are GPIOB[4...7].
    # GPIOB4: SCU80[12] = 0 and Strap[14] = 0
    # GPIOB5: SCU80[13] = 0
    # GPIOB6: SCU80[14] = 0
    # GPIOB7: SCU80[15] = 0
    devmem_clear_bit $(scu_addr 70) 14
    devmem_clear_bit $(scu_addr 80) 12
    devmem_clear_bit $(scu_addr 80) 13
    devmem_clear_bit $(scu_addr 80) 14
    devmem_clear_bit $(scu_addr 80) 15

    gpio_set 12 $bit0
    gpio_set 13 $bit1
    gpio_set 14 $bit2
    gpio_set 15 $bit3
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

