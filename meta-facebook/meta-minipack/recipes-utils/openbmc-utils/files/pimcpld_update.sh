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
UNIT=$1
IMAGE="$2"

if [ "${UNIT}" -le 0 ] || [ "${UNIT}" -ge 9 ]; then
    echo "PIM #${UNIT}: not a valid PIM Unit"
    exit -1
fi

source /usr/local/bin/openbmc-utils.sh

PIN_OFFSETS=(3 4 5 6)
SHADOWS=(BMC_PIM${UNIT}_CPLD_TCK BMC_PIM${UNIT}_CPLD_TMS \
         BMC_PIM${UNIT}_CPLD_TDO BMC_PIM${UNIT}_CPLD_TDI)
BUS=$(((${UNIT} - 1) * 8 + 85 ))
PCA9534=`i2cdetect -y ${BUS} 0x26 0x26 | grep "\-\-"` > /dev/null

# PIM1 -> BUS 85 ; PIM2 -> BUS 93 ; PIM3 -> BUS 101; PIM4 -> BUS 109
# PIM5 -> BUS 117; PIM6 -> BUS 125; PIM7 -> BUS 133; PIM8 -> BUS 141
pca9534_gpio_add()
{
    if [ "${PCA9534}" != "" ]; then
        echo "PIM #${UNIT} not insert or PCA9534 fail!"
        exit -1
    fi

    i2c_device_add ${BUS} 0x26 pca9534
    usleep 100000

    for i in {0..3}; do
        gpio_export_by_offset ${BUS}-0026 ${PIN_OFFSETS[i]} ${SHADOWS[i]}
    done
}

pca9534_gpio_delete()
{
    if [ "${PCA9534}" != "" ]; then
        exit -1
    fi

    for i in {0..3}; do
        gpio_unexport ${SHADOWS[i]}
    done

    i2c_device_delete ${BUS} 0x26
    rm -rf /tmp/pimcpld_update
}

trap pca9534_gpio_delete INT TERM QUIT EXIT

# export pca9534 GPIO to connect BMC to PIM CPLD pins
pca9534_gpio_add ${UNIT}

echo 1 > /tmp/pimcpld_update

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${IMAGE}" \
    --tms ${SHADOWS[1]} \
    --tdo ${SHADOWS[2]} \
    --tdi ${SHADOWS[3]} \
    --tck ${SHADOWS[0]}
result=$?
# 1 is returned upon upgrade success
if [ $result -eq 1 ]; then
    echo "Upgrade successful."
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    exit 1
fi
