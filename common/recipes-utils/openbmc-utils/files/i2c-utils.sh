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

SYSFS_I2C_DEVICES="/sys/bus/i2c/devices"
SYSFS_I2C_DRIVERS="/sys/bus/i2c/drivers"

#
# instantiate an i2c device.
# $1 - parent bus number
# $2 - device address
# $3 - device name/type
#
i2c_device_add() {
    local bus
    local addr
    local device
    bus=$"$1"
    addr="$2"
    device="$3"
    echo ${device} ${addr} > /sys/class/i2c-dev/i2c-${bus}/device/new_device
}

#
# delete an i2c device.
# $1 - parent bus number
# $2 - device address
#
i2c_device_delete() {
    local bus
    local addr
    bus=$"$1"
    addr="$2"
    echo ${addr} > /sys/class/i2c-dev/i2c-${bus}/device/delete_device
}

#
# instantiate an i2c-mux device and wait untill all its child buses
# are initialized before the function returns.
# $1 - parent bus number
# $2 - i2c-mux device address
# $3 - i2c-mux device name/type
# $4 - bus number of the last i2c-mux channel
#
i2c_mux_add_sync() {
    retry=0
    max_retry=100
    bus_dir="/sys/class/i2c-dev/i2c-${4}"

    i2c_device_add ${1} ${2} ${3}

    until [ -d ${bus_dir} ]
    do
        usleep 2000 # sleep for 2 milliseconds

        retry=$((retry + 1))
        if [ $retry -ge ${max_retry} ]
        then
            echo "failed to create child buses for i2c-mux ${1}-${2}!"
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
i2c_driver_bind_device() {
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
# The function goes through all i2c devices and check if some devices
# are not claimed by drivers (due to scheduling delay, missing drivers,
# intermittent i2c transaction errors in device_probe() function, and
# etc.).
# If callers pass "force_binding" parameter, then the function will
# trigger manual driver binding for devices without drivers.
# $1 - force binding driver
#
i2c_check_driver_binding() {
    local total_devs=0
    local errors=0
    local errors_fixed=0
    local force_binding=$1
    local sum_info error_info
    local driver_name dev_name dev_path

    echo "checking i2c driver binding status"
    for filename in `ls ${SYSFS_I2C_DEVICES}`; do
        if [[ ${filename} == *-00* ]]; then
            total_devs=$((total_devs + 1))

            dev_path="${SYSFS_I2C_DEVICES}/${filename}"
            if [ -L "${dev_path}/driver" ]; then
                continue # driver binded successfully
            fi

            errors=$((errors + 1))
            dev_name=`cat ${dev_path}/name`
            echo "${filename} (${dev_name}) - no driver binded"

            if [ -z ${force_binding} ] ||
               [ ${force_binding} != "force_binding" ]; then
                continue
            fi

            driver_name=$(i2c_driver_map ${dev_name})
            if [ -z ${driver_name} ]; then
                echo "unable to find driver for ${filename}"
                continue
            fi

            echo "manually binding ${filename} to driver ${driver_name}"
            i2c_driver_bind_device ${driver_name} ${filename}
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

#
# Return the given i2c device's absolute sysfs path.
#
i2c_device_sysfs_abspath() {
    local i2c_dev=$1
    echo "${SYSFS_I2C_DEVICES}/${i2c_dev}"
}
