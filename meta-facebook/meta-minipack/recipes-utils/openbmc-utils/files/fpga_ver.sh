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
#


echo "------IOBFPGA------"
iob_ver=`head -n 1 /sys/class/i2c-adapter/i2c-13/13-0035/fpga_ver 2> /dev/null`
iob_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-13/13-0035/fpga_sub_ver 2> /dev/null`

if [ $? -eq 0 ]; then
    echo "IOBFPGA: $(($iob_ver)).$(($iob_sub_ver))"
else
    echo "IOBFPGA is not detected"
fi

num=1
BUS="80 88 96 104 112 120 128 136"

echo "------DOMFPGA------"

for bus in ${BUS}; do
    pim_type=`head -n1 /sys/class/i2c-adapter/i2c-${bus}/${bus}-0060/board_ver 2> /dev/null`

    echo "PIM $num:"
    if [ "${pim_type}" == "0x0" ]; then
        dom_ver=`head -n 1 /sys/class/i2c-adapter/i2c-${bus}/${bus}-0060/fpga_ver 2> /dev/null`
        dom_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-${bus}/${bus}-0060/fpga_sub_ver 2> /dev/null`
        echo "16Q DOMFPGA: $(($dom_ver)).$(($dom_sub_ver))"
    elif [ "${pim_type}" == "0x10" ]; then
        dom_ver=`head -n 1 /sys/class/i2c-adapter/i2c-${bus}/${bus}-0060/fpga_ver 2> /dev/null`
        dom_sub_ver=`head -n 1 /sys/class/i2c-adapter/i2c-${bus}/${bus}-0060/fpga_sub_ver 2> /dev/null`
        echo "4DD DOMFPGA: $(($dom_ver)).$(($dom_sub_ver))"
    else
        echo "DOMFPGA is not detected or PIM $num is not inserted"
    fi

    num=$(($num+1))
    usleep 50000
done
