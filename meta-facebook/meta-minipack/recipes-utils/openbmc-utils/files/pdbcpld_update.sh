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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

board_rev=$(wedge_board_rev)

usage(){
    program=`basename "$0"`
    echo "Usage:"
    echo "     $program jtag <Location> <*.vme>"
    echo "     $program i2c <Location> <*.hex>"
    echo "     <Location>  : left or right"
}

if [ $board_rev -eq 4 ]; then
    echo "EVTA hardware doesn't have PDBCPLD"
    exit -1
fi

if [ "$#" -ne 3 ]; then
    usage
    exit -1
fi

if [ "$2" = "left" ]; then
    IO_BUS=54
    CPLD_BUS=53
elif [ "$2" = "right" ]; then
    IO_BUS=62
    CPLD_BUS=61
else
    echo "$2: not a valid PDB location"
    exit -1
fi

interface="$1"
img="$3"

CPLD_PIN_OFFSETS=(0 1 2 3)
CPLD_SHADOWS=(BMC_PDB_CPLD_TDO BMC_PDB_CPLD_TDI BMC_PDB_CPLD_TMS BMC_PDB_CPLD_TCK)
HITLESS_OFFSET=7
HITLESS_SHADOW=BMC_PDB_HITLESS
PCA9534=`i2cdetect -y ${IO_BUS} 0x21 0x21 | grep "\-\-"` > /dev/null

pca9534_gpio_add()
{
    if [ "${PCA9534}" != "" ]; then
        echo "PDB $1 not insert or PCA9534 fail!"
        exit -1
    fi

    i2c_device_add ${IO_BUS} 0x21 pca9534
    usleep 100000

    gpio_export_by_offset ${IO_BUS}-0021 ${HITLESS_OFFSET} ${HITLESS_SHADOW}
    gpio_set ${HITLESS_SHADOW} 0

    if [ "$interface" = "jtag" ]; then
        for i in {0..3}; do
            gpio_export_by_offset ${IO_BUS}-0021 ${CPLD_PIN_OFFSETS[i]} ${CPLD_SHADOWS[i]}
        done
    fi
}

pca9534_gpio_delete()
{
    if [ "${PCA9534}" != "" ]; then
        exit -1
    fi

    gpio_set ${HITLESS_SHADOW} 1
    gpio_unexport ${HITLESS_SHADOW}

    if [ "$interface" = "jtag" ]; then
        for i in {0..3}; do
            gpio_unexport ${CPLD_SHADOWS[i]}
        done
    fi


    i2c_device_delete ${IO_BUS} 0x21
    rm -rf /tmp/pdbcpld_update
}

trap pca9534_gpio_delete INT TERM QUIT EXIT

echo 1 > /tmp/pdbcpld_update

# export pca9534 GPIO to connect BMC to PDB CPLD pins
pca9534_gpio_add $2

if [ "$interface" = "jtag" ]; then
    ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" \
              --tms ${CPLD_SHADOWS[2]} --tdo ${CPLD_SHADOWS[0]} \
              --tdi ${CPLD_SHADOWS[1]} --tck ${CPLD_SHADOWS[3]}
    result=$?
    sleep 1
    # 1 is returned upon upgrade success
    if [ $result -eq 1 ]; then
        echo "Upgrade successful."
        exit 0
    else
        echo "Upgrade failure. Return code from utility : $result"
        exit 1
    fi
elif [ "$interface" = "i2c" ]; then
    cpldupdate-i2c ${CPLD_BUS} 0x40 "${img}"
    sleep 1
else
    echo "$1 is not a valid PDB interface"
    exit -1
fi
