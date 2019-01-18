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

. /usr/local/bin/openbmc-utils.sh

dump_fpga_version() {
    local fpga_dir=$1
    local fpga_name=$2

    fpga_ver=`head -n 1 ${fpga_dir}/fpga_ver 2> /dev/null`
    if [ $? -ne 0 ]; then
        echo "${fpga_name} is not detected"
        return
    fi
    fpga_sub_ver=`head -n 1 ${fpga_dir}/fpga_sub_ver 2> /dev/null`
    if [ $? -ne 0 ]; then
        echo "${fpga_name} is not detected"
        return
    fi

    echo "${fpga_name}: $(($fpga_ver)).$(($fpga_sub_ver))"
}

echo "------IOBFPGA------"
dump_fpga_version ${IOBFPGA_SYSFS_DIR} "IOBFPGA"

echo "------DOMFPGA------"
for num in $(seq 8); do
    pim_var_name="PIM${num}_DOMFPGA_SYSFS_DIR"
    pim_fpga_dir=${!pim_var_name}

    echo "PIM $num:"

    pim_type=`head -n1 ${pim_fpga_dir}/board_ver 2> /dev/null`
    if [ "${pim_type}" == "0x0" ]; then
        dump_fpga_version ${pim_fpga_dir} "16Q_DOMPFGA"
    elif [ "${pim_type}" == "0x10" ]; then
        dump_fpga_version ${pim_fpga_dir} "4DD_DOMPFGA"
    else
        echo "DOMFPGA is not detected or PIM $num is not inserted"
    fi
    usleep 50000
done
