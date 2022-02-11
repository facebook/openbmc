#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

# The T2 chip can prefer different input voltages, depending, presumably
# of manufacturing variations.  We need to determine whether it wants
# 0.95V or 1.025V, reset the T2 to reduce total power usage, set the
# outgoing voltage on the first buck converter, and bring T2 up out of
# reset.

. /usr/local/fbpackages/utils/ast-functions

kernel_ver=$(get_kernel_ver)

# read the T2 ROV after the GPIOs are enabled
t2_rov() {
    local val0 val1 val2
    # Note that the values are *not* read in order.
    if [ $kernel_ver == 5 ]; then
        val0=$(cat /sys/class/gpio/gpio850/value 2>/dev/null)
        val1=$(cat /sys/class/gpio/gpio848/value 2>/dev/null)
        val2=$(cat /sys/class/gpio/gpio849/value 2>/dev/null)
    else
        val0=$(cat /sys/class/gpio/gpio58/value 2>/dev/null)
        val1=$(cat /sys/class/gpio/gpio56/value 2>/dev/null)
        val2=$(cat /sys/class/gpio/gpio57/value 2>/dev/null)
    fi
    echo $((val0 | (val1 << 1) | (val2 << 2)))
}

rov=$(t2_rov)

# target_volts come from the data sheet and 18mV of loss and
# some fudging based on actual measurements to get either 1.025V
# or 0.95V at T2
if [ $rov -eq 1 ]; then
    target_volts=0x5a
elif [ $rov -eq 2 ]; then
    target_volts=0x65
else
    echo "Unrecognized T2 ROV value $rov, setting failed."
    exit 1
fi
target_volts=$(( $target_volts * 1 ))  # normalize to decimal

# We shouldn't have to rmmod pmbus, because it hasn't been loaded yet,
# but if the script is rerun after the system is up, it may be necessary.
if [ $kernel_ver == 4 ]; then
    rmmod pmbus
    reload=$?
fi

# Get current voltage value
cur_volts=$(i2cget -y 1 0x60 0x8b w)
cur_volts=$(( $cur_volts * 1 ))  # normalize to decimal

# Only bounce the T2 if we actually need to modify the voltage
if [ $cur_volts -ne $target_volts ]; then 
    # Set values before turning out output;  we're using "PCIE, then MCS"
    gpio_set SYSTEM_ID3_LED_N F2 1
    gpio_set SYSTEM_ID4_LED_N F3 1
    gpio_set SMB_TEMP_SCL_R C0 0
    # T2 is in reset;  note that this may cause NMI messages on the uServer,
    # which shouldn't be up anyway when this is first run.
    
    # Set the requested value to the current value to avoid rapid shifts
    i2cset -y 1 0x60 0x21 $cur_volts w
    # Enable the requested voltage
    i2cset -y 1 0x60 0xd2 0x5a
    i2cset -y 1 0x60 0xd3 0x5a
    sleep 1

    # Set the target voltage
    i2cset -y 1 0x60 0x21 $target_volts w
    
    sleep 1

    # Let T2 come out of reset
    gpio_set SMB_TEMP_SCL_R C0 1
    echo "T2 ROV value set based on $rov."
    sleep 2
    gpio_set SYSTEM_ID3_LED_N F2 0
    gpio_set SYSTEM_ID4_LED_N F3 0
else
    echo "T2 ROV already correctly set."
fi

if [ $kernel_ver == 4 ]; then
    # Bring back pmbus if necessary
    if [ $reload -eq 0 ]; then
        modprobe pmbus
    fi
fi
