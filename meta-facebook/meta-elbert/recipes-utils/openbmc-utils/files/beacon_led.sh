#!/bin/bash
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

# led-controld looks for this file; if found, beacon LED will be on.
BEACON_FILE="/tmp/.elbert_beacon"

beacon_led_on() {
     if [ -f "$BEACON_FILE" ]
     then
          echo "Beacon LED is already on."
          exit 1
     fi

     touch "$BEACON_FILE"
}

beacon_led_off() {
     if [ ! -f "$BEACON_FILE" ]
     then
          echo "Beacon LED is already off."
          exit 1
     fi

     rm -rf "$BEACON_FILE"
}

usage() {
     program=$(basename "$0")
     echo "Usage:"
     echo "$program [on|off]"
     exit 1
}

command="$1"
shift

case "$command" in
     on)
          beacon_led_on
          ;;
     off)
          beacon_led_off
          ;;
     *)
          usage
          ;;
esac
