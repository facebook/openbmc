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

echo -n "Setup sensor monitoring for fby2... "

# Check for the slots present and run sensord for those slots only.
# check is_bic_ready for Server Cards only, remove this check for DC
SLOTS=
  ret=0
  if [ $(is_server_prsnt 1) == "1" ]; then
    if [ $(get_slot_type 1) == "0" ]; then
      ret=$(is_bic_ready 1);
    else
      ret=1;
    fi

    if [ $ret == "1" ]; then
      SLOTS="$SLOTS slot1"
    fi
  fi

  if [ $(is_server_prsnt 2) == "1" ]; then
    if [ $(get_slot_type 2) == "0" ]; then
      ret=$(is_bic_ready 2);
    else
      ret=1;
    fi

    if [ $ret == "1" ]; then
      SLOTS="$SLOTS slot2"
    fi
  fi

  if [ $(is_server_prsnt 3) == "1" ]; then
    if [ $(get_slot_type 3) == "0" ]; then
      ret=$(is_bic_ready 3);
    else
      ret=1;
    fi

    if [ $ret == "1" ]; then
      SLOTS="$SLOTS slot3"
    fi
  fi

  if [ $(is_server_prsnt 4) == "1" ]; then
    if [ $(get_slot_type 4) == "0" ]; then
      ret=$(is_bic_ready 4);
    else
      ret=1;
    fi

    if [ $ret == "1" ]; then
      SLOTS="$SLOTS slot4"
    fi
  fi

SLOTS="$SLOTS spb nic"
/usr/local/bin/sensord $SLOTS

echo "done."
