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

### BEGIN INIT INFO
# Provides:          setup-sensord
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup sensor monitoring
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

echo -n "Setup gpio monitoring for fby2... "

# Check for the slots present and run sensord for those slots only.
SLOTS=
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 1) == "1" && $(get_slot_type 1) == "0" ]]; then
    SLOTS="$SLOTS slot1"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 2) == "1" && $(get_slot_type 2) == "0" ]]; then
    SLOTS="$SLOTS slot2"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 3) == "1" && $(get_slot_type 3) == "0" ]]; then
    SLOTS="$SLOTS slot3"
  fi
  #Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point)
  #get_slot_type is to get slot type to check if the slot type is server
  if [[ $(is_server_prsnt 4) == "1" && $(get_slot_type 4) == "0" ]]; then
    SLOTS="$SLOTS slot4"
  fi

/usr/local/bin/gpiod $SLOTS

echo "done."
