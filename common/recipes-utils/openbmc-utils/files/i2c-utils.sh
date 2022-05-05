# Copyright 2015-present Facebook. All Rights Reserved.
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

SYSFS_I2C_ROOT="/sys/bus/i2c"
SYSFS_I2C_DEVICES="${SYSFS_I2C_ROOT}/devices"
SYSFS_I2C_DRIVERS="${SYSFS_I2C_ROOT}/drivers"

#
# Return the given i2c device's absolute sysfs path.
# $1 - i2c device in <bus>-00<add> format (for example 0-0050).
#
i2c_device_sysfs_abspath() {
    echo "${SYSFS_I2C_DEVICES}/${1}"
}

#
# instantiate an i2c device.
# $1 - parent bus number
# $2 - device address
# $3 - device name/type
#
i2c_device_add() {
    bus=$"$1"
    addr="$2"
    device="$3"
    echo "$device" "$addr" > "${SYSFS_I2C_DEVICES}/i2c-${bus}/new_device"
}

#
# delete an i2c device.
# $1 - parent bus number
# $2 - device address
#
i2c_device_delete() {
    bus=$"$1"
    addr="$2"
    echo "$addr" > "${SYSFS_I2C_DEVICES}/i2c-${bus}/delete_device"
}

#
# Create message-queue i2c slave backend for the given i2c master.
# $1 - i2c bus number
# $2 - slave device address
#
i2c_mslave_add() {
    bus=$1
    addr=$2

    addr=$((addr | 0x1000))
    i2c_device_add "$bus" "$addr" slave-mqueue
}

#
# instantiate an i2c-mux device and wait untill all its child buses
# are initialized before the function returns.
# $1 - parent bus number
# $2 - i2c-mux device address
# $3 - i2c-mux device name/type
# $4 - total number of channels provided by the i2c-mux
#
i2c_mux_add_sync() {
    bus="$1"
    addr="$2"
    name="$3"
    nchans="$4"

    i2c_device_add "$bus" "$addr" "$name"

    retry=0
    format_addr=$(printf "%04x" "$addr")
    dev_entry="${bus}-${format_addr}"
    nchans=$((nchans - 1))
    last_channel="${SYSFS_I2C_DEVICES}/${dev_entry}/channel-${nchans}"
    until [ -d "${last_channel}" ]; do
        usleep 10000 # sleep for 10 milliseconds

        retry=$((retry + 1))
        if [ "$retry" -ge 10 ]; then
            echo "failed to create child buses for i2c-mux $dev_entry!"
            return 1
        fi
    done

    return 0
}

#
# Lookup i2c driver based on device name.
# $1 - device name
#
i2c_driver_map() {
    local device_name=$1

    case ${device_name} in
        "24c64" | "24c32" | "24c16" | "24c08" | "24c04" | "24c02" | "24c01" | "24c00")
            echo "at24";;
        "adm1278")
            echo "adm1275";;
        "adm1266")
            echo "adm1266";;
        "tmp75" | "lm75" | "tmp1075")
            echo "lm75";;
        "tmp421" | "tmp422")
            echo "tmp421";;
        "pca9505" | "pca9534" | "pca9535")
            echo "pca953x";;
        "pca9548")
            echo "pca954x";;
        "pfe1100" | "pfe3000")
            echo "bel-pfe";;
        "fancpld")
            echo "fancpld";;
        "cmmcpld")
            echo "cmmcpld";;
        "mp2975" | "mp2978")
            echo "mp2975";;
        "lm25066")
            echo "lm25066";;
        "ucd90160" | "ucd90124")
            echo "ucd9000";;
        "xdpe132g5c")
            echo "xdpe132g5c";;
        "ir35215")
            echo "ir35215";;
        "xdpe12284")
            echo "xdpe12284";;
        "pxe1211")
            echo "pxe1610";;
        *)
            echo "";;
    esac
}

#
# Trigger driver binding manually
# $1 - driver name
# $2 - device path in <bus>-00<addr> format.
# $3 - retry count, optional argument (default 1: no retry).
#
i2c_bind_driver() {
    driver_name="$1"
    i2c_device="$2"
    driver_dir="${SYSFS_I2C_DRIVERS}/${driver_name}"

    if [ -n "$3" ]; then
        retries="$3"
    else
        retries=1
    fi

    if [ ! -d ${driver_dir} ]; then
        echo "unable to locate i2c driver ${driver_name} in sysfs"
        return 1
    fi

    retry=0
    while [ "$retry" -lt "$retries" ]; do
        if echo "${i2c_device}" > "${driver_dir}/bind"; then
            return 0
        fi

        usleep 50000  # sleep for 50 milliseconds
        retry=$((retry + 1))
    done

    return 1
}

#
# The function goes through all i2c devices and check if some devices
# are not claimed by drivers (due to scheduling delay, missing drivers,
# intermittent i2c transaction errors in device_probe() function, and
# etc.).
# $1 - "fix-binding" flag. Optional argument: if $1 is "fix-binding",
#      the function will try to lookup drivers and fix driver binding.
#      Otherwise, nothing is done for "missing-driver" devices.
# $2 - retry count. It defines number of retries in case of driver
#      binding failures.
#
i2c_check_driver_binding() {
    errors=0
    errors_fixed=0
    total_devs=0

    if [ -n "$1" ] && [ "$1" = "fix-binding" ]; then
        fix_binding=1
    fi
    if [ -n "$2" ]; then
        retries="$2"
    else
        retries=1
    fi

    echo "checking i2c driver binding status"
    for dev_path in "${SYSFS_I2C_DEVICES}"/*-00*; do
        total_devs=$((total_devs + 1))

        if [ -L "${dev_path}/driver" ]; then
            continue # bound to driver successfully
        fi

        errors=$((errors + 1))
        dev_uid=$(basename "$dev_path")
        dev_name=$(cat "${dev_path}/name")
        echo "${dev_uid} (${dev_name}) - no driver bound"

        # Try to manually bind the device.
        if [ -n "${fix_binding}" ]; then
            driver_name=$(i2c_driver_map "${dev_name}")
            if [ -z "${driver_name}" ]; then
                echo "unable to find driver for ${dev_uid}"
                continue
            fi

            echo "manually bind ${dev_uid} to driver ${driver_name}"
            if i2c_bind_driver "${driver_name}" "${dev_uid}" "$retries"; then
                errors_fixed=$((errors_fixed + 1))
            fi
        fi
    done

    # Dump driver binding summary.
    errors=$((errors - errors_fixed))
    sum_info="total ${total_devs} i2c devices checked"
    if [ ${errors} -eq 0 ]; then
        result_info="all bound to drivers"
    else
        result_info="${errors} devices without drivers"
    fi
    if [ "$errors_fixed" -gt 0 ]; then
        result_info="${result_info} ($errors_fixed manually fixed)"
    fi
    echo "${sum_info}: ${result_info}"
}
