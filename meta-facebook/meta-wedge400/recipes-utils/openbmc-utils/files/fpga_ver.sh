#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

exit_code=0

dump_fpga_version() {
    local bus=$1
    local addr=$2
    local fpga_name=$3

    if ! fpga_ver=$(i2cget -f -y "$bus" "$addr" 0x01 2>/dev/null); then
        echo "${fpga_name} is not detected"
        exit_code=1
        return
    fi

    if ! fpga_sub_ver=$(i2cget -f -y "$bus" "$addr" 0x02 2>/dev/null); then
        echo "${fpga_name} is not detected"
        exit_code=1
        return
    fi

    echo "${fpga_name}: $((fpga_ver)).$((fpga_sub_ver))"
}

dump_fpga_version 13 0x60 "DOMFPGA1"
dump_fpga_version 5 0x60 "DOMFPGA2"

if [ "$exit_code" -ne 0 ]; then
    echo "Not all DOMFPGA or PIM were detected/inserted. Please review the logs above.... exiting"
    exit 1
fi
