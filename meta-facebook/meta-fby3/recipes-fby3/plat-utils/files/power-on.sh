#!/bin/sh
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

### BEGIN INIT INFO
# Provides:          power-on
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Power on Server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

function init_class2_server() {
  if [ $(is_bmc_por) -eq 1 ]; then
    devmem_clear_bit $(scu_addr 9c) 17
    power-util slot1 on
  fi
}

function init_class1_server() {
  # Check whether it is fresh power on reset
  if [ $(is_bmc_por) -eq 1 ]; then
    # Disable clearing of PWM block on WDT SoC Reset
    devmem_clear_bit $(scu_addr 9c) 17

    if [ $(is_server_prsnt 1) == "1" ] ; then
      power-util slot1 on
    fi

    if [ $(is_server_prsnt 2) == "1" ] ; then
      power-util slot2 on
    fi

    if [ $(is_server_prsnt 3) == "1" ] ; then
      power-util slot3 on
    fi

    if [ $(is_server_prsnt 4) == "1" ] ; then
      power-util slot4 on
    fi
  fi
}

if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
  date -s "2018-01-01 00:00:00"
fi

/usr/local/bin/sync_date.sh

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_server
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_server
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi
