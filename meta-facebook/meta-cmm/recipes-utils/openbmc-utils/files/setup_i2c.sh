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

SYSFS_I2C_DEVICES="/sys/bus/i2c/devices"
SYSFS_I2C_DRIVERS="/sys/bus/i2c/drivers"

#
# instantiate an i2c device
# $1 - device name/type
# $2 - device address
# $3 - parent bus number
#
add_device() {
    echo ${1} ${2} > /sys/class/i2c-dev/i2c-${3}/device/new_device
}

#
# instantiate an i2c-mux device and wait untill all its child buses
# are initialized before the function returns.
# $1 - device name/type
# $2 - device address
# $3 - parent bus number
# $4 - bus number of the last i2c-mux channel
#
add_i2c_mux_sync() {
    retry=0
    max_retry=100
    bus_dir="/sys/class/i2c-dev/i2c-${4}"

    add_device ${1} ${2} ${3}

    until [ -d ${bus_dir} ]
    do
        usleep 2000 # sleep for 2 milliseconds

        retry=$((retry + 1))
        if [ $retry -ge ${max_retry} ]
        then
            echo "i2c-mux ${3}-${2} initialization failed: timer expired"
            exit 1
        fi
    done
}

#
# instantiate all the i2c-muxes.
# Note:
#   - the step is not needed in kernel 4.1 because all the i2c-muxes
#     are created in 4.1 kernel code. For details please refer to
#     "arch/arm/plat-aspeed/dev-i2c-cmm.c"
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
            add_i2c_mux_sync ${i2c_mux_name} ${addr} ${bus} ${last_child_bus}
        done
    done

    # Create second-level i2c-muxes which are connected to first level
    # mux "8-0077". "8-0077" has 8 channels and the first 4 channels are
    # connected with 1 extra level of i2c-mux (@ address 0x70), thus
    # totally 32 i2c buses (168-199) will be registered.
    parent_buses="32 33 34 35"
    for bus in ${parent_buses}; do
        last_child_bus=$((last_child_bus + 8))
        add_i2c_mux_sync ${i2c_mux_name} 0x70 ${bus} ${last_child_bus}
    done
}

#
# Lookup i2c driver based on device name.
# $1 - device name
#
i2c_lookup_driver() {
    local dev_name=$1

    case ${dev_name} in
        "24c64")
            echo "at24";;
        "tmp75")
            echo "lm75";;
        "pca9505" | "pca9534" | "pca9535")
            echo "pca953x";;
        "pca9548")
            echo "pca954x";;
        "pfe3000")
            echo "pfe3000";;
        "fancpld")
            echo "fancpld";;
        "cmmcpld")
            echo "cmmcpld";;
        *)
            echo "";;
    esac
}

#
# Trigger driver binding manually
# $1 - driver name
# $2 - device path in <bus>-<addr> format.
#
manual_driver_binding() {
    local retry=0
    local retry_max=2
    local sleep_cnt=0
    local sleep_max=20
    local driver_name=${1}
    local i2c_dev=${2}
    local driver_dir="${SYSFS_I2C_DRIVERS}/${driver_name}"
    local driver_link="${SYSFS_I2C_DEVICES}/${i2c_dev}/driver"

    if [ ! -d ${driver_dir} ]; then
        echo "unable to locate i2c driver ${driver_name} in sysfs"
        return 1
    fi

    until [ -L ${driver_link} ]
    do
        if [ $retry -ge ${retry_max} ]; then
            echo "failed to bind ${i2c_dev} after ${retry} retries"
            return 1
        fi
        echo ${i2c_dev} > "${driver_dir}/bind" 2> /dev/null

        sleep_cnt=0
        while [ ! -L ${driver_link} ] && [ ${sleep_cnt} -lt ${sleep_max} ]
        do
            usleep 500 # sleep for 500 microseconds
            sleep_cnt=$((sleep_cnt + 1))
        done

        retry=$((retry + 1))
    done

    echo "i2c driver ${driver_name} binded to ${i2c_dev} successfully"
    return 0
}

#
# The function goes through all i2c devices and manually trigger driver
# binding if device binding failed/delayed. It's needed because:
#   1) i2c device creation and driver binding happens asynchronous,
#      which means a device may not be claimed by its driver when
#      add_device() returns. In this case, We need to wait for a short
#      period so device manager gets chance to run and complete driver
#      binding.
#   2) We have seen some intermittent i2c transaction errors which leads
#      to device probe failure. The root cause is not clear yet, but for
#      now, let's try to bind the device again.
#
fixup_i2c_driver_binding() {
    local total_devs=0
    local errors=0
    local errors_fixed=0
    local sum_info error_info
    local driver_name dev_name dev_path

    echo "checking i2c driver binding status"

    # sleep 50 milliseconds just in case driver binding is "delayed".
    usleep 50000

    for filename in `ls ${SYSFS_I2C_DEVICES}`; do
        if [[ ${filename} == *-00* ]]; then
           total_devs=$((total_devs + 1))

           dev_path="${SYSFS_I2C_DEVICES}/${filename}"
           if [ -L "${dev_path}/driver" ]; then
              continue # driver binded successfully
           fi

           errors=$((errors + 1))

           dev_name=`cat ${dev_path}/name`
           driver_name=$(i2c_lookup_driver ${dev_name})
           if [ -z ${driver_name} ]; then
               echo "unable to find driver for ${filename}"
               continue
           fi

           echo "manually binding ${filename} to driver ${driver_name}"
           manual_driver_binding ${driver_name} ${filename}
           if [ "$?" -eq "0" ]; then
               errors_fixed=$((errors_fixed + 1))
           fi
        fi
    done

    sum_info="total ${total_devs} i2c devices checked"
    if [ ${errors} -eq 0 ]; then
        err_info="no errors found"
    else
        err_info="${errors} errors found, ${errors_fixed} errors fixed"
    fi
    echo "${sum_info}: ${err_info}"
}

KERNEL_VERSION=`uname -r`
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    bulk_create_i2c_mux
fi

# EEPROM
add_device 24c64 0x51 6

#PCB EEPROM
add_device 24c64 0x50 28

# Two temperature sensors on CMM
add_device tmp75 0x48 3         # inlet
add_device tmp75 0x49 3         # outlet

# CMM CPLD
add_device cmmcpld 0x3e 13

# FAN CPLD
fancpld_busses="171 179 187 195"
for bus in ${fancpld_busses}; do
    add_device fancpld 0x33 ${bus}
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
        add_device tmp75 ${addr} ${bus}
    done
done

# Temperature sensors on SCM
temp_busses="113 129 49 65 81 97 145 161"
for bus in ${temp_busses}; do
    add_device tmp75 0x48 ${bus}
    add_device tmp75 0x49 $((bus + 1))
done

# FCB PCA9534
# GPIOs are allocated backwards.
# So, list the bus list in reverse order,
# so that GPIO numbers are increasing from FCB1-FCB4
fcb_busses="196 188 180 172"
for bus in ${fcb_busses}; do
    add_device pca9534 0x22 ${bus}
done

# PSU PRESENCE
# Spec says pca9506 but this kernel version supports only 9505
# which works for us
add_device pca9505 0x20 29

# PFE modules
add_device pfe3000 0x10 27 # psu1
add_device pfe3000 0x10 26 # psu2
add_device pfe3000 0x10 25 # psu3
add_device pfe3000 0x10 24 # psu4

# LEDs
add_device pca9535 0x21 30

#
# Go through all i2c devices and trigger manual driver binding if needed.
#
if [[ ${KERNEL_VERSION} != 4.1.* ]]; then
    fixup_i2c_driver_binding
fi
