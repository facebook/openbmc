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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

SWPLD1_DIR=$SWPLD1_SYSFS_DIR


function usage
{
  program=$(basename "$0")
  echo "Usage:"
  echo "     $program "
  echo "     $program <TYPE>"
  echo "     <TYPE>: fan/psu"
}

# Will return <key> : <val>
# eg - scm : 1
function get_presence
{
  local file=$1 key=$2
  val=$(head -n1 "$file" | awk '{print substr($0,0,3) }' | sed "s/.x//")
  if [ "$val" = "0" ]; then
      val=1
  else
      val=0
  fi
  echo "$key : $val"
}

# FANs
function get_fan_presence
{
  local num=1
  while [ $num -le 6 ]; do
      key="fan${num}"
      file="/tmp/gpionames/FAN${num}_PRESENT/value"
      get_presence "$file" "$key"
      (( num++ ))
  done
}

# Power supplies
function get_psu_presence
{
  for i in {1..2}; do
      key="psu${i}"
      file="$SWPLD1_DIR/psu${i}_present"
      get_presence "$file" "$key"
  done
}

# get all info
if [[ $1 = "" ]]; then
  echo "FANS"
  get_fan_presence

  echo "POWER SUPPLIES"
  get_psu_presence

  exit 0
else
  if [[ $1 = "fan" ]]; then
    get_fan_presence
  elif [[ $1 = "psu" ]]; then
    get_psu_presence
  else
    usage
  fi
  exit 0
fi
