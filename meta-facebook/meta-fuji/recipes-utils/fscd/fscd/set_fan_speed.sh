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
#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..8)> " >&2
}

set -e

# This util is called by fscd system service as a pre-condition
# Therefore, if fscd thermal model is not configured, do that here
if ! [ -f /etc/fsc-config.json ]; then
  # This is the target file that the config will be written
  default_fsc_config="/etc/fsc-config.json"
  if [ -f /etc/netwhoami ]; then
    # If we already have netwhoami, use that
    echo "Checking netwhoami for detecting role."
    role=$(/bin/grep "role=" /etc/netwhoami | /usr/bin/cut -d '=' -f2|/usr/bin/tr "[:upper:]" "[:lower:]")
  fi
  if [ "$role" == "" ]; then
    # If role is still not found, check if cached hostname exists
    if [ -f /mnt/data/hostname ]; then
      echo "Using the cached hostname to infer the role."
      role=$(/bin/grep -o '[a-z]\+' /mnt/data/hostname | /usr/bin/head -n 1)
    fi
  fi
  if [ "$role" == "" ]; then
    # Finally, infer the role from the current hostname
    if [ -f /etc/hostname ]; then 
      echo "Using the hostname to infer the role."
      role=$(/bin/grep -o '[a-z]\+' /etc/hostname | /usr/bin/head -n 1)
    fi
  fi
  case $role in
    rtsw)
      profile="/etc/fsc-config-rtsw.json"
      ;;
    rsw)
      profile="/etc/fsc-config-non-rtsw.json"
      ;;
    fsw)
      profile="/etc/fsc-config-non-rtsw.json"
      ;;
    ssw)
      profile="/etc/fsc-config-non-rtsw.json"
      ;;
    fuji)
      profile="/etc/fsc-config-non-rtsw.json"
      ;;
    fboss)
      profile="/etc/fsc-config-non-rtsw.json"
      ;;
    *)
      profile="/etc/fsc-config-rtsw.json"
      ;;
  esac
  echo "Setting up the thermal profile for role: $role. Filename: $profile"
  # shellcheck disable=SC2154
  /bin/cp $profile $default_fsc_config
fi

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# check percent argument be number
if ! [[ $1 =~ ^[0-9]+$ ]]; then
    echo "Percent argument can be only integer number"
    usage
    exit 1
fi
# check percent in range 0-100
if [ $1 -lt 0 ] || [ $1 -gt 100 ]; then
    echo "Percent argument out of range"
    usage
    exit 1
fi

# Top FCM    (BUS=64) PWM 1 ~ 4 control Fan 1 3 5 7
# Bottom FCM (BUS=72) PWM 1 ~ 4 control Fan 2 4 6 8
BUS="64 72"
if [ "$#" -eq 1 ]; then
    FAN_UNIT="1 2 3 4"
else
    if [ "$2" -le 0 ] || [ "$2" -ge 9 ]; then
        echo "Fan $2: not a valid Fan Unit"
        exit 1
    fi

    FAN="$2"

    if [ $((FAN % 2)) -eq 0 ]; then
      BUS="72"
      FAN_UNIT=$((FAN / 2 ))
    else
      BUS="64"
      FAN=$((FAN / 2 ))
      FAN_UNIT=$((FAN + 1))
    fi
fi
    # Convert the percentage to our 1/64th level (0-63).
    step=$(( ($1 * 630 + 5) / 1000 ))
cnt=1

for bus in ${BUS}; do
    for unit in ${FAN_UNIT}; do
        pwm="$(i2c_device_sysfs_abspath ${bus}-0033)/fan${unit}_pwm"
        echo "$step" > "${pwm}"

    if [ "$#" -eq 1 ]; then
        echo "Successfully set fan $cnt speed to $1%"
        cnt=$((cnt+1))
    else
        echo "Successfully set fan $2 speed to $1%"
    fi
    done
done
