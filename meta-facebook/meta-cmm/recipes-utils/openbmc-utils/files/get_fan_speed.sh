#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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
usage() {
  echo "Usage: $0 [Fan Unit (1...12)]" >&2
}

# map the output of the command into an array
mapfile -t FAN_ARRAY < <(sensors fancpld-*|grep ^Fan)

show_rpm(){
  front_fan_idx=$(( ($1 - 1) * 2 ))
  rear_fan_idx=$((( ( $1 - 1) * 2) + 1))
  IFS=' ' read -r -a fan_front <<< "${FAN_ARRAY[$front_fan_idx]}"
  IFS=' ' read -r -a fan_rear <<< "${FAN_ARRAY[$rear_fan_idx]}"

  if [ "${#fan_front[@]}" -ne 5 ] || [ "${#fan_rear[@]}" -ne 5 ]; then
      echo "wrong sensors fan reading input. Exiting ..."
      exit 1
  fi

  echo "${fan_front[3]}, ${fan_rear[3]}"
}

# Return appropriate value based on the argument passed to it.
if [ "$#" -eq 0 ]; then
  FANS="1 2 3 4 5 6 7 8 9 10 11 12"
elif [ "${#}" -eq 1 ]; then
    if [ "${1}" -lt 1 ] || [ "${1}" -gt 12 ]; then
        usage
        exit 1
    fi
    FANS="$1"
else
    usage
    exit 1
fi

for fan in $FANS; do
  echo "Fan $fan RPMs: $(show_rpm "${fan}")"
done
