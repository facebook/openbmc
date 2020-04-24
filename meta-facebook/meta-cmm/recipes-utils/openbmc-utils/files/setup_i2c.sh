#!/bin/sh
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

##### NOTE #########################################################
# - When adding new devices, always add to end of the list here
#   so any other dependencies that are hard coded with GPIO
#   will not break.
# - Although technically all the i2c devices can be created by using
#   device tree, we prefer to do it here, mainly because:
#     * it's easier to control the order of device creation.
#     * a majority of i2c devices are located on line/fabric cards,
#       and it's easier to control device creation/removal and add
#       hot-plug support in the future.
####################################################################

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/i2c-utils.sh

#
# instantiate all the i2c-muxes.
# Note:
#   - the function may be skipped if i2c switches were already created
#     in kernel space.
#   - please do not modify the order of i2c-mux creation unless you've
#     decided to fix bus-number of leaf i2c-devices.
#
bulk_create_i2c_mux() {
    last_child_bus=15
    i2c_mux_name="pca9548"

    # The first-level i2c-muxes which are directly connected to aspeed
    # i2c adapters are described in device tree, and the bus number of
    # these i2c-muxes' channels are hardcoded (as below) to avoid
    # breaking the existing applications.
    #    i2c-mux 1-0077: child bus 16-23
    #    i2c-mux 2-0071: child bus 24-31
    #    i2c-mux 8-0077: child bus 32-39

    # Create second-level i2c-muxes which are connected to first level
    # mux "1-0077". "1-0077" has 8 channels and each channel is connected
    # with 2 more i2c-muxes (@ address 0x70 & 0x73), thus total 128 i2c
    # buses (40-167) will be registered.
    parent_buses="18 19 20 21 16 17 22 23"
    mux_addresses="0x70 0x73"
    for bus in ${parent_buses}; do
        for addr in ${mux_addresses}; do
            last_child_bus=$((last_child_bus + 8))
            i2c_mux_add_sync ${bus} ${addr} ${i2c_mux_name} ${last_child_bus}
        done
    done

    # Create second-level i2c-muxes which are connected to first level
    # mux "8-0077". "8-0077" has 8 channels and the first 4 channels are
    # connected with 1 extra level of i2c-mux (@ address 0x70), thus
    # totally 32 i2c buses (168-199) will be registered.
    parent_buses="32 33 34 35"
    for bus in ${parent_buses}; do
        last_child_bus=$((last_child_bus + 8))
        i2c_mux_add_sync ${bus} 0x70 ${i2c_mux_name} ${last_child_bus}
    done
}

#
# Let's check a few i2c switches on fabric cards to determine if these
# switches were already created in kernel space.
#
I2C_MUX_FAB1="${SYSFS_I2C_DEVICES}/16-0070"
I2C_MUX_FAB2="${SYSFS_I2C_DEVICES}/17-0070"
I2C_MUX_FAB3="${SYSFS_I2C_DEVICES}/22-0070"
I2C_MUX_FAB4="${SYSFS_I2C_DEVICES}/23-0070"
if [ ! -e "${I2C_MUX_FAB1}" ] && [ ! -e "${I2C_MUX_FAB2}" ] && \
   [ ! -e "${I2C_MUX_FAB3}" ] && [ ! -e "${I2C_MUX_FAB4}" ]; then
    bulk_create_i2c_mux
fi

# EEPROM
i2c_device_add 6 0x51 24c64

#PCB EEPROM
i2c_device_add 28 0x50 24c64

# Two temperature sensors on CMM
i2c_device_add 3 0x48 tmp75 # inlet
i2c_device_add 3 0x49 tmp75 # outlet

# CMM CPLD
i2c_device_add 13 0x3e cmmcpld

# FAN CPLD
fancpld_busses="171 179 187 195"
for bus in ${fancpld_busses}; do
    i2c_device_add ${bus} 0x33 fancpld
done

# Temperature sensors
# FAB1: 106   0x48 - 0x4b
# SCM-FAB1: 113 0x48, 114 0x49

# FAB2: 122
# SCM-FAB2: 129 0x48, 130 0x49

# LC101: 42, 0x48-0x4b
# LC102: 45, 0x48-0x4b
# SCM-LC1: 49 0x48, 50 0x49

# LC201: 58
# LC202: 61
# SCM-LC2: 65 0x48, 66, 0x49

# LC301: 74
# LC302: 77
# SCM-LC3: 81 0x48, 82, 0x49

# LC401: 90
# LC401: 93
# SCM-LC4: 97 0x48, 98 0x49

# FAB3: 138
# SCM-FAB3: 145 0x48, 146 0x49

# FAB4: 154
# SCM-FAB4: 161 0x48, 162 0x49

# Temperature sensors on LC/FC
temp_busses="42 45 58 61 74 77 90 93 106 122 138 154"
temp_addrs="0x48 0x49 0x4a 0x4b"
for bus in ${temp_busses}; do
    for addr in ${temp_addrs}; do
        i2c_device_add ${bus} ${addr} tmp75
    done
done

# Temperature sensors on SCM
temp_busses="113 129 49 65 81 97 145 161"
for bus in ${temp_busses}; do
    i2c_device_add ${bus} 0x48 tmp75
    i2c_device_add $((bus + 1)) 0x49 tmp75
done

# FCB PCA9534
# GPIOs are allocated backwards.
# So, list the bus list in reverse order,
# so that GPIO numbers are increasing from FCB1-FCB4
fcb_busses="196 188 180 172"
for bus in ${fcb_busses}; do
    i2c_device_add ${bus} 0x22 pca9534
done

# PSU PRESENCE
# Spec says pca9506 but this kernel version supports only 9505
# which works for us
i2c_device_add 29 0x20 pca9505

# PFE modules
i2c_device_add 27 0x10 pfe3000 # psu1
i2c_device_add 26 0x10 pfe3000 # psu2
i2c_device_add 25 0x10 pfe3000 # psu3
i2c_device_add 24 0x10 pfe3000 # psu4

# LEDs
i2c_device_add 30 0x21 pca9535

#
# Go through all i2c devices and trigger manual driver binding if needed.
#
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    # sleep 50 milliseconds just in case driver binding is "delayed".
    usleep 50000

    i2c_check_driver_binding "fix-binding"
fi
