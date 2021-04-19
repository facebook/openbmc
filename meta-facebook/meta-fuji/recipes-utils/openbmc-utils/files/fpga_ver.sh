#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh
exitCode=0

dump_fpga_version() {
    local fpga_dir=$1
    local fpga_name=$2

    fpga_ver=$(head -n 1 "${fpga_dir}/fpga_ver" 2> /dev/null)
    if [ $? -ne 0 ]; then
        echo "${fpga_name} is not detected"
        exit 1

    fi
    fpga_sub_ver=$(head -n 1 "${fpga_dir}/fpga_sub_ver" 2> /dev/null)
    if [ $? -ne 0 ]; then
        echo "${fpga_name} is not detected"
        exit 1
    fi

    echo "${fpga_name}: $((fpga_ver)).$((fpga_sub_ver))"
}

echo "------IOBFPGA------"
dump_fpga_version "${IOBFPGA_SYSFS_DIR}" "IOBFPGA"

echo "------DOMFPGA------"
for num in $(seq 8); do
    pim_var_name="PIM${num}_DOMFPGA_SYSFS_DIR"
    pim_fpga_dir=${!pim_var_name}

    echo "PIM $num:"

    pim_type=$(head -n1 "${pim_fpga_dir}/board_ver" 2> /dev/null)
    if [ "$((pim_type & 0x80))" == 0 ]; then
        dump_fpga_version "${pim_fpga_dir}" "16Q DOMFPGA"
    elif [ "$((pim_type & 0x80))" == "$((0x80))" ]; then
        dump_fpga_version "${pim_fpga_dir}" "16O DOMFPGA"
    else
        echo "DOMFPGA is not detected or PIM $num is not inserted"
        exitCode=1
    fi
    usleep 50000
done

if [ "$exitCode" -ne 0 ]; then
    echo "Not all DOMFPGA or PIM were detected/inserted. Please review the logs above.... exiting"
    exit 1
fi

