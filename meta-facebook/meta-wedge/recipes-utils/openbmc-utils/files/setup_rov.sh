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

. /usr/local/bin/openbmc-utils.sh

PMBUS_DRIVER_DIR="${SYSFS_I2C_DRIVERS}/pmbus"

# read the T2 ROV after the GPIOs are enabled
t2_rov() {
    local val0 val1 val2
    # Note that the values are *not* read in order.
    val0=$(gpio_get_value T2_ROV3)
    val1=$(gpio_get_value T2_ROV1)
    val2=$(gpio_get_value T2_ROV2)
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
target_volts=$((target_volts * 1))  # normalize to decimal

#
# Unbind pmbus driver from 1-0060, otherwise, following i2cget could
# fail with EBUSY.
#
echo 1-0060 > "${PMBUS_DRIVER_DIR}/unbind"

# Get current voltage value.
# The device here is NCP4200. It turns out the first i2c transaction to this
# device always fails. The spec does not mention anything about NCP4200 i2c
# engine start delay. However, each time, when the isolation buffer between the
# BMC i2c controller and NCP4200 is disabled and re-enabled, the first i2c
# transaction to NCP4200 always fails.
# To workaround this issue, we are doing the first read twice.
cur_volts=$(i2cget -y 1 0x60 0x8b w 2> /dev/null)
cur_volts=$(i2cget -y 1 0x60 0x8b w)
cur_volts=$((cur_volts * 1))  # normalize to decimal

# Only bounce the T2 if we actually need to modify the voltage
if [ $cur_volts -ne $target_volts ]; then
    # Set values before turning out output;  we're using "PCIE, then MCS"
    gpio_set_value RESET_SEQ1 1
    gpio_set_value RESET_SEQ0 1
    gpio_set_direction RESET_SEQ1 out
    gpio_set_direction RESET_SEQ0 out
    gpio_set_value T2_RESET_N 0
    gpio_set_direction T2_RESET_N out
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
    gpio_set_value T2_RESET_N 1
    echo "T2 ROV value set based on $rov."
    sleep 2
    gpio_set_value RESET_SEQ1 0
    gpio_set_value RESET_SEQ0 0
else
    echo "T2 ROV already correctly set."
fi

#
# Bind pmbus driver to 1-0060.
#
echo 1-0060 > "${PMBUS_DRIVER_DIR}/bind"
