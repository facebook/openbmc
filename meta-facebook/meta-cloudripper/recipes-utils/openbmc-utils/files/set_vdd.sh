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

#shellcheck disable=SC1091
. /usr/local/bin/i2c-utils.sh

usage()
{
  echo "usage :"
  echo "   $0 <voltage in mv>"
  echo "   eg. $0 850"
  echo " limit : max 930 mV"
}

find_hwmon_path() {
  cd "$1"/hwmon/hwmon* 2> /dev/null || return 1
  pwd
}

if [ $# -ne 1 ]; then
  usage
  exit 255
fi

if [ $(($1)) -gt 931 ]; then
  usage
  exit 255
fi

vddcore_i2c_path=$(i2c_device_sysfs_abspath 17-0040)
vddcore_hwmon_path=$(find_hwmon_path "$vddcore_i2c_path")
if [ -z "$vddcore_hwmon_path" ]; then
  echo "device is not found"
  exit 1
fi
vddcore_sysfs_node="$vddcore_hwmon_path/vout0_value"

if echo "$1" > "$vddcore_sysfs_node"; then
  echo "$1"
  exit 0
else
  echo "setting error"
  exit 254
fi
