#!/bin/sh
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

. /usr/local/bin/openbmc-utils.sh

usage() {
    echo "Usage: $0 <PERCENT (0..100)> <Fan Unit (1..5)> " >&2
}

FAN_DIR=$(i2c_device_sysfs_abspath 8-0033)

# This util is called by fscd system service as a pre-condition
# Therefore, if fscd thermal model is not configured, do that here
# This is the target file that the config will be written
default_fsc_config="/etc/fsc-config.json"
if [ -f /etc/netwhoami ]; then
  # If we already have netwhoami, use that
  echo "Checking netwhoami for detecting the datacenter location."
  dc=$(/bin/grep "network_name=" /etc/netwhoami | /bin/grep -o '[a-z]\+[0-9]' | /usr/bin/tail -n 1 |grep -o '[a-z]\+')
fi
# shellcheck disable=SC2039
if [ "$dc" == "" ]; then
  # If role is still not found, check if cached hostname exists
  if [ -f /mnt/data/hostname ]; then
    echo "Using the cached hostname to infer the datacenter location."
    dc=$(/bin/grep -o '[a-z]\+[0-9]' /mnt/data/hostname | /usr/bin/tail -n 1 |grep -o '[a-z]\+')
  fi
fi
# shellcheck disable=SC2039
if [ "$dc" == "" ]; then
  # Finally, infer the role from the current hostname
  if [ -f /etc/hostname ]; then
    echo "Using the hostname to infer the datacenter location."
    dc=$(/bin/grep -o '[a-z]\+[0-9]' /etc/hostname | /usr/bin/tail -n 1 |grep -o '[a-z]\+')
  fi
fi
case $dc in
  vll)
    profile="/etc/fsc-config-vll.json"
    ;;
  *)
    profile="/etc/fsc-config.json"
    ;;
esac
echo "Setting up the thermal profile for datacenter: $dc. Filename: $profile"
# shellcheck disable=SC2039
if [ "$profile" != "$default_fsc_config" ]; then
  if [ -f $profile ]; then 
    /bin/cp -f $profile $default_fsc_config
  else 
    echo "Unable to find the config file. Using the default file"
  fi
else
  echo "Using the default fscd config file"
fi

set -e

if [ "$#" -ne 2 ] && [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

if [ "$#" -eq 1 ]; then
    FANS="1 2 3 4 5"
else
    FANS="$2"
fi

# Convert the percentage to our 1/32th unit (0-31).
unit=$(( ( $1 * 31 ) / 100 ))

for fan in $FANS; do
    pwm="${FAN_DIR}/fantray${fan}_pwm"
    echo "$unit" > $pwm
    echo "Successfully set fan ${fan} speed to $1%"
done
