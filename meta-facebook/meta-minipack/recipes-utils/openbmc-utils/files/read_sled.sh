#!/bin/bash
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

IO0=$(i2cget -f -y 50 0x20 0x2)
IO1=$(i2cget -f -y 50 0x20 0x3)

usage(){
  program=$(basename "$0")
    echo "Usage:"
    echo "     $program <brd_rev>"
    echo ""
    echo "     <brd_rev>: borad revision"
}

print_color_name(){
  case $2 in
      6)
          color="red"
      ;;
      3)
          color="blue"
      ;;
      4)
          color="yellow"
      ;;
      7)
          color="off"
      ;;
      *)
      echo "Read value that is not a valid color $1:$2"
      exit 1
      ;;
  esac
  echo "$1:$color"
}

if [ "$1" = "0" ] || [ "$1" = "4" ]; then

    PSU_VAL=$(((IO0 & 0x38) >> 3))
    SMB_VAL=$(((IO1 & 0x38) >> 3))
    FAN_VAL=$((IO0 & 0x7))
    SYS_VAL=$((IO1 & 0x7))

else 

    FAN_VAL=$(((IO0 & 0x38) >> 3))
    SMB_VAL=$(((IO1 & 0x38) >> 3))
    SYS_VAL=$((IO0 & 0x7))
    PSU_VAL=$((IO1 & 0x7))

fi

print_color_name "sys" $SYS_VAL
print_color_name "smb" $SMB_VAL
print_color_name "psu" $PSU_VAL
print_color_name "fan" $FAN_VAL

