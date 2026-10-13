#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

SC_POWERGOOD="/sys/bus/i2c/devices/9-0023/smb_power_status"
RUGGLES_CPU_ID=8

cpu_id=0
for i in 0 1 2 3; do
	val="$(gpiocli get-value --shadow "CPU_ID_$i" 2>/dev/null)"
	case "${val%"${val##*[![:space:]]}"}" in
	*1) cpu_id=$((cpu_id | (1 << i))) ;;
	esac
done

echo -e "\n##### WEDGE CPU ID #####"
echo "$cpu_id"

echo -e "\n##### USER PWR STATUS #####"
/usr/local/bin/wedge_power.sh status

# Ruggles has no switchcard, so no powergood sysfs to read.
if [ "$cpu_id" != "$RUGGLES_CPU_ID" ]; then
	echo -e "\n##### SWITCHCARD POWERGOOD STATUS #####"
	if [ ! -f "$SC_POWERGOOD" ]; then
		echo "$SC_POWERGOOD doesn't exist!"
	else
		head -n 1 "$SC_POWERGOOD"
	fi
fi
