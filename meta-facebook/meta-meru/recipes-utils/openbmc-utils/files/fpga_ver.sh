#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

maj_ver="cpld_ver_major"
min_ver="cpld_ver_minor"
exitCode=0

echo "------SCM-FPGA------"

if [ ! -d "$PWRCPLD_SYSFS_DIR" ]; then
    echo "SCM_FPGA: FPGA_DRIVER_NOT_DETECTED"
    exitCode=1
else
    val_major=$(head -n 1 "$PWRCPLD_SYSFS_DIR"/"$maj_ver")
    val_minor=$(head -n 1 "$PWRCPLD_SYSFS_DIR"/"$min_ver")
    echo "SCM_FPGA: $((val_major)).$((val_minor))"
fi

if [ "$exitCode" -ne 0 ]; then
    echo "One or more fpga versions was not detected... exiting"
    exit 1
fi
