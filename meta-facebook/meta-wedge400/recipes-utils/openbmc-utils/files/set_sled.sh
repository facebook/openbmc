#!/bin/sh
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

# Set all IO direction to output
i2cset -f -y 6 0x20 0x06 0x00
i2cset -f -y 6 0x20 0x07 0x00

IO0=$(i2cget -f -y 6 0x20 0x2)
IO1=$(i2cget -f -y 6 0x20 0x3)

color=$2

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "     $program <dev> <color>"
    echo ""
    echo "     <dev>: sts/smb/psu/fan"
    echo "     <color>: off/red/blue/yellow/green"
}

case $color in
    "red")
        val=0x6
    ;;
    "blue")
        val=0x3
    ;;
    "yellow")
        val=0x4
    ;;
    "green")
        val=0x5
    ;;
    "off")
        val=0x7
    ;;
    *)
    usage
    exit 1
    ;;
esac

if [ "$3" = "0" ] || [ "$3" = "4" ]; then

  if [ "$1" = "smb" ] || [ "$1" = "fan" ]; then
    val=$((val << 3))
    IO0_VAL=$(((IO0 & 0x7) | val))
    IO1_VAL=$(((IO1 & 0x7) | val))
  elif [ "$1" = "sts" ] || [ "$1" = "psu" ]; then
    IO0_VAL=$(((IO0 & 0x38) | val))
    IO1_VAL=$(((IO1 & 0x38) | val))
  else
    usage
    exit 1
  fi

  if [ "$1" = "sts" ] || [ "$1" = "fan" ]; then
    i2cset -f -y 6 0x20 0x2 $IO0_VAL
  else
    i2cset -f -y 6 0x20 0x3 $IO1_VAL
  fi

else 

  if [ "$1" = "fan" ] || [ "$1" = "smb" ]; then
    val=$((val << 3))
    IO0_VAL=$(((IO0 & 0x7) | val))
    IO1_VAL=$(((IO1 & 0x7) | val))
  elif [ "$1" = "sts" ] || [ "$1" = "psu" ]; then
    IO0_VAL=$(((IO0 & 0x38) | val))
    IO1_VAL=$(((IO1 & 0x38) | val))
  else	
    usage
    exit 1
  fi

  if [ "$1" = "fan" ] || [ "$1" = "sts" ]; then
    i2cset -f -y 6 0x20 0x2 $IO0_VAL
  else
    i2cset -f -y 6 0x20 0x3 $IO1_VAL
  fi

fi

