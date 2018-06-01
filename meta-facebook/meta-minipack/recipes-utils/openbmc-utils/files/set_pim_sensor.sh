#!/bin/sh
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

. /usr/local/bin/openbmc-utils.sh

usage(){
    program=`basename "$0"`
    echo "Usage:"
    echo "     $program all <Operation>"
    echo "     $program <PIM> <Operation>"
    echo "     <PIM>: 1 ~ 8"
    echo "     <Operation>: load / unload / id"
}

if [ $# -ne 2 ]; then
    usage
    exit -1
fi

count=0

# Bus 80 88 96 104 112 120 128 136   # PIM1 ~ PIM8 FPGA
# Bus 82 90 98 106 114 122 130 138   # PIM1 ~ PIM8 temp. sensor
# Bus 83 91 99 107 115 123 131 139   # PIM1 ~ PIM8 temp. sensor
# Bus 84 92 100 108 116 124 132 140  # PIM1 ~ PIM8 Hotswap
# Bus 86 94 102 110 118 126 134 142  # PIM1 ~ PIM8 MAX34461
if [ "$1" == "all" ]; then
    FPGA_BUS="80 88 96 104 112 120 128 136"
else
    if [ "$1" -le 0 ] || [ "$1" -ge 9 ]; then
        echo "PIM$1: not a valid PIM Unit"
        exit -1
    fi
    FPGA_BUS=$((($1 - 1) * 8 + 80 ))
fi

if [ "$2" == "unload" ]; then
    for bus in ${FPGA_BUS}; do
        i2c_device_delete $(($bus + 4)) 0x10 2> /dev/null
        i2c_device_delete $(($bus + 6)) 0x74 2> /dev/null
        i2c_device_delete $(($bus + 2)) 0x48 2> /dev/null
        i2c_device_delete $(($bus + 3)) 0x4b 2> /dev/null
    done
elif [ "$2" == "load" ]; then
    #Sleep to avoid mount max34461 fail.
    #It needs 3~4 seconds to initialize.
    #Thus, we sleep 5 seconds to be on the save side.
    sleep 5
    for bus in ${FPGA_BUS}; do

        i2c_device_add $(($bus + 4)) 0x10 adm1278 2> /dev/null
        i2c_device_add $(($bus + 6)) 0x74 max34461 2> /dev/null
        i2c_device_add $(($bus + 2)) 0x48 tmp75 2> /dev/null
        i2c_device_add $(($bus + 3)) 0x4b tmp75 2> /dev/null
        
    done

elif [ "$2" == "id" ]; then
    for bus in ${FPGA_BUS}; do
        pim_16q=`i2cdetect -y $bus 0x60 0x60 | grep "\-\-"` > /dev/null
        pim_4dd=`i2cdetect -y $bus 0x61 0x61 | grep "\-\-"` > /dev/null
        slot=$(($(($(($bus - 80)) / 8)) + 1))
        if [ "${pim_16q}" != "" ] && [ "${pim_4dd}" != "" ]; then
            if [ "$1" == "all" ]; then
                continue
            else
                exit -1
            fi
        elif [ "${pim_16q}" == "" ] && [ "${pim_4dd}" != "" ]; then
            ret=`i2cget -y -f $bus 0x60 0x03` 2> /dev/null
            #set PIM number into DOM FPGA 0x03 register.
            if [[ "$ret" -ne "0x0$slot" ]]; then
                i2cset -f -y $bus 0x60 0x03 $1
            fi
        else
            ret=`i2cget -y -f $bus 0x61 0x03` 2> /dev/null
            #set PIM number into DOM FPGA 0x03 register.
            if [[ "$ret" -ne "0x0$slot" ]]; then
                i2cset -f -y $bus 0x61 0x03 $1
            fi
        fi
    done
else
    echo "Wrong operation"
    usage
    exit -1
fi
