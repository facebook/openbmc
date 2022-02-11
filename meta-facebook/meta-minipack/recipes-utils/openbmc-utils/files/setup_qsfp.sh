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
    echo "     $program reset <PIM>"
    echo "     $program data  <PIM> <Operation>"
    echo "     <PIM>: all, 1 ~ 8"
    echo "     <Operation>: enable / disable"
}

if [ $# -le 1 ] || [ $# -ge 4 ]; then
    usage
    exit -1
fi

# Bus 80 88 96 104 112 120 128 136   # PIM1 ~ PIM8 FPGA
if [ "$2" == "all" ]; then
    FPGA_BUS="80 88 96 104 112 120 128 136"
elif [ "$2" -ge 1 ] || [ "$2" -le 8 ]; then
    FPGA_BUS=$((($2 - 1) * 8 + 80 ))
else
    usage
    exit -1 
fi

if [ "$1" == "reset" ]; then
    echo -n "Reset QSFP ... "
    for bus in ${FPGA_BUS}; do
        i2c_path=$(i2c_device_sysfs_abspath ${bus}-0060)
        reset="${i2c_path}/qsfp_reset"
        echo 0xffff > "${reset}"
        usleep 1000
        echo 0x0 > "${reset}"
    done
    echo "Done"
elif [ "$1" == "data" ]; then
    echo -n "DOM data collection "$3" ... "
    for bus in ${FPGA_BUS}; do
        i2c_path=$(i2c_device_sysfs_abspath ${bus}-0060)
        config="${i2c_path}/dom_ctrl_config"
        if [ "$3" == "enable" ]; then
            echo 0x5000001 > "${config}"
        elif [ "$3" == "disable" ]; then
            echo 0xf00 > "${config}"
        else
            usage
            exit -1
        fi
    done
    echo "Done"
else
    usage
    exit -1
fi
