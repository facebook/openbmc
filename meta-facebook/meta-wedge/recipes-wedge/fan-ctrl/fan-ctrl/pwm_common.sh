#!/bin/sh
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
set -e

HWMON_SYSFS_ROOT="/sys/class/hwmon"

#
# Lookup pwm sysfs directory, used in kernel 5.x or higher versions.
#
hwmon_lookup_by_name() {
    hwmon_name="$1"
    pwm_dir=""

    for entry in "${HWMON_SYSFS_ROOT}"/*; do
        if echo "$entry" | grep "/hwmon/hwmon" > /dev/null 2>&1; then
            if grep "$hwmon_name" "${entry}/name" > /dev/null 2>&1; then
                pwm_dir="$entry"
                break
            fi
        fi
    done

    echo "$pwm_dir"
}
