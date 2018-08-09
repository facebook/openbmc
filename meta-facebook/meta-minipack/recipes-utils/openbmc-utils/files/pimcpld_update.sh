#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

usage(){
    program=`basename "$0"`
    echo "Usage:"
    echo "     $program <PIM> <File Path>"
    echo "     <PIM>: 1 ~ 8"
}

if [ $# -ne 2 ]; then
    usage
    exit -1
fi

if [ "$1" -le 0 ] || [ "$1" -ge 9 ]; then
    echo "PIM$1: not a valid PIM Unit"
    exit -1
fi

source /usr/local/bin/openbmc-utils.sh

img="$2"
PINS=(483 484 485 486)
BUS=$((($1 - 1) * 8 + 85 ))
PCA9534=`i2cdetect -y ${BUS} 0x26 0x26 | grep "\-\-"` > /dev/null

# PIM1 -> BUS 85 ; PIM2 -> BUS 93 ; PIM3 -> BUS 101; PIM4 -> BUS 109
# PIM5 -> BUS 117; PIM6 -> BUS 125; PIM7 -> BUS 133; PIM8 -> BUS 141
pca9534_gpio_add()
{
    if [ "${PCA9534}" != "" ]; then
        echo "PIM$1 not insert or PCA9534 fail!"
        exit -1
    fi

    i2c_device_add ${BUS} 0x26 pca9534
    usleep 100000

    for i in {0..3}; do
        echo "${PINS[i]}" > /sys/class/gpio/export
    done
}

pca9534_gpio_delete()
{
    if [ "${PCA9534}" != "" ]; then
        exit -1
    fi

    for i in {0..3}; do
        echo "${PINS[i]}" > /sys/class/gpio/unexport
    done

    i2c_device_delete ${BUS} 0x26
    rm -rf /tmp/pimcpld_update
}

trap pca9534_gpio_delete INT TERM QUIT EXIT

# export pca9534 GPIO to connect BMC to PIM CPLD pins
pca9534_gpio_add $1

echo 1 > /tmp/pimcpld_update

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" --tms ${PINS[1]} --tdo ${PINS[2]} --tdi ${PINS[3]} --tck ${PINS[0]}
