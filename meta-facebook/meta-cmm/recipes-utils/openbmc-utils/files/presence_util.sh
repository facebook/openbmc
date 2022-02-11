#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

source /usr/local/bin/openbmc-utils.sh

SYSFS_I2C_DEVICES='/sys/bus/i2c/devices'

CMD="${SYSFS_I2C_DEVICES}/13-003e/*present"
FAN_CMD1="${SYSFS_I2C_DEVICES}/171-0033/"
FAN_CMD2="${SYSFS_I2C_DEVICES}/179-0033/"
FAN_CMD3="${SYSFS_I2C_DEVICES}/187-0033/"
FAN_CMD4="${SYSFS_I2C_DEVICES}/195-0033/"

# Will return <key> : <val>
# eg :- cmm : 1
function get_presence
{
  local file=$1 key=$2
  val=$(head -n1 $file | awk '{print substr($0,0,3) }' | sed 's/.x//')
  if [ $val -eq 0 ]; then
    val=1
  else
    val=0
  fi
  echo $key ":" $val
}

# CMMs, FCs, LCs, SCMs
function get_card_presence
{
  for file in $CMD
  do
    key=$(echo $file | sed 's/.*003e\///' | sed 's/_present.*//')
    get_presence $file $key
  done
}

# If fc1 is present fcb1 is present, same for other fcbs
# Fans
function get_fan_presence
{
  local num=1 fan_cmd=$1 count=$((($2 * 3) - 2))
  while [ $num -lt 4 ]; do
    key='fan'$count
    file=$fan_cmd'fantray'$num'_present'
    get_presence $file $key
    (( num++ ))
    (( count++ ))
  done
}

# Power supplies
function get_psu_presence
{
  local num=1
  while [ $num -lt 5 ]; do
    key='psu'$num
    cmd='gpio_get PSU'$num'_PRESENT'
    val=$($cmd)
    if [ $val -eq 0 ]; then
      val=1
    else
      val=0
    fi
    echo $key ":" $val
    (( num++ ))
  done
}

# get all info
echo "CARDS"
get_card_presence

echo "FANS"
get_fan_presence $FAN_CMD1 1
get_fan_presence $FAN_CMD2 2
get_fan_presence $FAN_CMD3 3
get_fan_presence $FAN_CMD4 4

echo "POWER SUPPLIES"
get_psu_presence
