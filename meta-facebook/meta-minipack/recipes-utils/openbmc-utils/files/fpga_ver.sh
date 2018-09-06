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

if [ $? -eq 1 ]; then
    echo "IOBFPGA is not detected"
else
    echo "IOBFPGA: $(($iob_ver)).$(($iob_sub_ver))"
fi

num=1
BUS="80 88 96 104 112 120 128 136"

echo "------DOMFPGA------"

for bus in ${BUS}; do
    pim_16q=`i2cdetect -y $bus 0x60 0x60 | grep "\-\-" 2> /dev/null`
    pim_4dd=`i2cdetect -y $bus 0x61 0x61 | grep "\-\-" 2> /dev/null`

    echo "PIM $num:"

    if [ "${pim_16q}" != "" ] && [ "${pim_4dd}" != "" ]; then
        echo "DOMFPGA is not detected or PIM $num is not inserted"
    elif [ "${pim_16q}" == "" ] && [ "${pim_4dd}" != "" ]; then
        dom_16q_ver=$(i2cget -f -y ${bus} 0x60 0x01)
        dom_16q_sub_ver=$(i2cget -f -y ${bus} 0x60 0x02)
        echo "16Q DOMFPGA: $(($dom_16q_ver)).$(($dom_16q_sub_ver))"
    else
        dom_4dd_ver=$(i2cget -f -y ${bus} 0x61 0x01)
        dom_4dd_sub_ver=$(i2cget -f -y ${bus} 0x61 0x02)
        echo "4DD DOMFPGA: $(($dom_4dd_ver)).$(($dom_4dd_sub_ver))"
    fi

    num=$(($num+1))
    usleep 50000
done