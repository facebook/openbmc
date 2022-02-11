#!/bin/sh
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

dev=$1
color=$2

usage(){
  program=$(basename "$0")
  echo "Usage:"
  echo "     $program <dev> <color>"
  echo ""
  echo "     <dev>: sys/fan/psu/smb"
  echo "     <color>: off/red/blue/green/amber"
}

led=""
case $dev in
  "sys" | "fan" | "psu" | "smb")
    led="/sys/class/leds/$dev"
  ;;
  *)
    usage
    exit 1
  ;;
esac

# color: red/green/blue value: 0-255
# brightness: 0-255
brightness="255"
case $color in
  "red")
    color="255 0 0"
  ;;
  "blue")
    color="0 0 255"
  ;;
  "green")
    color="0 255 0"
  ;;
  "amber")
    color="255 45 0"
  ;;
  "off")
    color="0 0 0"
    brightness=0
  ;;
  *)
    usage
    exit 1
  ;;
esac

echo "$color" > "$led/multi_intensity"
echo $brightness > "$led/brightness"
