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

# This enum is defined in led-controld.h; do not make changes here without
# also updating that file.
BEACON_MODE_OFF=-1
BEACON_MODE_LOCATOR=0
BEACON_MODE_NETSTATE=1
BEACON_MODE_DRAINED=2
BEACON_MODE_AUDIT=3

beacon_led_on() {
     mode_str=$1

     if [ -z "$mode_str" ]; then
          mode_str="locator"
     fi

     # Translate mode to int for file storage.
     mode_int="$BEACON_MODE_OFF"
     case "$mode_str" in
          locator)
               mode_int="$BEACON_MODE_LOCATOR"
               ;;
          netstate)
               mode_int="$BEACON_MODE_NETSTATE"
               ;;
          drained)
               mode_int="$BEACON_MODE_DRAINED"
               ;;
          audit)
               mode_int="$BEACON_MODE_AUDIT"
               ;;
          *)
               usage
               ;;
     esac

     if [ -f "$BEACON_FILE" ]; then
          curr_mode=$(<"$BEACON_FILE")
          if [[ "$mode_int" == "$curr_mode" ]]; then
               echo "Beacon LED: already ON in $mode_str mode"
               exit 1
          fi
     fi

     echo "$mode_int" > "$BEACON_FILE"
     echo "Beacon LED: now ON in $mode_str mode"
}

beacon_led_off() {
     if [ ! -f "$BEACON_FILE" ]; then
          echo "Beacon LED: already OFF"
          exit 1
     fi

     rm -rf "$BEACON_FILE"
     echo "Beacon LED: now OFF"
}

usage() {
     program=$(basename "$0")
     echo "Usage:"
     echo "$program <on|off> [locator | netstate | drained | audit]"
     exit 1
}

command="$1"
shift

case "$command" in
     on)
          beacon_led_on "$1"
          ;;
     off)
          beacon_led_off
          ;;
     *)
          usage
          ;;
esac
