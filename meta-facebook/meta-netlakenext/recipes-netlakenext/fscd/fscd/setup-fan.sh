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

### BEGIN INIT INFO
# Provides:          setup-fan
# Required-Start:    board-id
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Set fan speed
### END INIT INFO

FAN_RPM_CFG="/mnt/data/kv_store/fan_rpm_cfg"
DEFAULT_FAN_SPEED=80

# check file exist and read fan speed value, remove all whitespace characters
if [ -s "$FAN_RPM_CFG" ]; then
  FAN_SPEED="$(tr -d '[:space:]' < "$FAN_RPM_CFG")"
fi

# check if fan speed is set, if not, set to default value
if [ -z "$FAN_SPEED" ]; then
  FAN_SPEED="$DEFAULT_FAN_SPEED"
  echo "No fan speed config found, fallback to default ${FAN_SPEED}%."
elif [[ "$FAN_SPEED" =~ ^-?[0-9]+$ ]]; then
  # pure number, use as is
  :
elif [[ "$FAN_SPEED" =~ ^(-?[0-9]+) ]]; then
  # number with special characters, extract the numeric part
  FAN_SPEED="${BASH_REMATCH[1]}"
  echo "Fan speed config has special characters, using extracted value ${FAN_SPEED}%."
else
  # completely non-numeric, log error and fallback to 100%
  logger -t "setup-fan" -p user.warning "Invalid fan speed config '$FAN_SPEED', fallback to 100%."
  FAN_SPEED="100"
  echo "Invalid fan speed config, fallback to 100%."
fi

# range check: <0 or >100 => force to 100
if (( FAN_SPEED < 0 || FAN_SPEED > 100 )); then
  logger -t "setup-fan" -p user.warning "Out-of-range(0~100%) fan speed '$FAN_SPEED', force to 100%."
  FAN_SPEED="100"
  echo "Fan speed out of range, force to 100%."
fi

echo "Current fan speed is set to ${FAN_SPEED}%."
/usr/local/bin/fan-util --set "$FAN_SPEED"
echo "Setup fan speed done."
