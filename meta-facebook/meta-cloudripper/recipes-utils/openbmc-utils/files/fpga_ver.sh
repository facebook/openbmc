#!/bin/bash
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

#shellcheck disable=SC1091,SC2034
. /usr/local/bin/openbmc-utils.sh

DOMFPGA1_SYSFS_DIR=$(i2c_device_sysfs_abspath 13-0060)
DOMFPGA2_SYSFS_DIR=$(i2c_device_sysfs_abspath 5-0060)

dump_fpga_version() {
    local fpga_dir=$1
    local fpga_name=$2

    fpga_ver=$(head -n 1 "$fpga_dir/fpga_ver" 2> /dev/null)
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "${fpga_name} is not detected"
        return
    fi

    fpga_sub_ver=$(head -n 1 "${fpga_dir}/fpga_sub_ver" 2> /dev/null)
    ret=$?
    if [ $ret -ne 0 ]; then
        echo "${fpga_name} is not detected"
        return
    fi

    echo "${fpga_name}: $((fpga_ver)).$((fpga_sub_ver))"
}

dump_fpga_version "$DOMFPGA1_SYSFS_DIR" DOMFPGA1
dump_fpga_version "$DOMFPGA2_SYSFS_DIR" DOMFPGA2
