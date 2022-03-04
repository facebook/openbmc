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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# Set all IO direction to output
i2cset -f -y 6 0x20 0x06 0x00
i2cset -f -y 6 0x20 0x07 0x00

IO0=$(i2cget -f -y 6 0x20 0x2)
IO1=$(i2cget -f -y 6 0x20 0x3)

color=$2
name=$1

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "     $program <dev> <color>"
    echo ""
    echo "     <dev>: sts/smb/psu/fan"
    echo "     <color>: off/red/blue/yellow/green"
}

brd_type_rev=$(wedge_board_type_rev)
# In wedge400 MP respin the led location are swaped
if [ "$brd_type_rev" = "WEDGE400_MP_RESPIN" ]; then
  case $name in
    "sts")
      led_id=2
    ;;
    "fan")
      led_id=4
    ;;
    "psu")
      led_id=3
    ;;
    "smb")
      led_id=1
    ;;
    *)
      usage
      exit 1
    ;;
  esac
else
  case $name in
    "sts")
      led_id=1
    ;;
    "fan")
      led_id=2
    ;;
    "psu")
      led_id=3
    ;;
    "smb")
      led_id=4
    ;;
    *)
      usage
      exit 1
    ;;
  esac
fi

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

if [ $((led_id)) -eq 2 ] || [ $((led_id)) -eq 4 ]; then
  val=$((val << 3))
  IO0_VAL=$(((IO0 & 0x7) | val))
  IO1_VAL=$(((IO1 & 0x7) | val))
elif [ $((led_id)) -eq 1 ] || [ $((led_id)) -eq 3 ]; then
  IO0_VAL=$(((IO0 & 0x38) | val))
  IO1_VAL=$(((IO1 & 0x38) | val))
else
  usage
  exit 1
fi

if [ $((led_id)) -eq 1 ] || [ $((led_id)) -eq 2 ]; then
  i2cset -f -y 6 0x20 0x2 $IO0_VAL
else
  i2cset -f -y 6 0x20 0x3 $IO1_VAL
fi


