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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions

DEF_PWR_ON=1
TO_PWR_ON=

check_por_config() {
  TO_PWR_ON=0

  if POR=$(kv get "slot${1}_por_cfg" persistent); then
    if [ "$POR" = "lps" ]; then
      if LS=$(kv get "pwr_server${1}_last_state" persistent); then
        if [ "$LS" = "on" ]; then
          TO_PWR_ON=1;
        fi
      else
        TO_PWR_ON=$DEF_PWR_ON
      fi
    elif [ "$POR" = "on" ]; then
      TO_PWR_ON=1;
    fi
  else
    TO_PWR_ON=$DEF_PWR_ON
  fi
}

init_class2_server() {
  if [ "$(is_bmc_por)" -eq 1 ]; then
    check_por_config 1
    if [ "$(is_sb_bic_ready 1)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
      power-util slot1 on
    fi
  fi

  show_sys_config -r > /dev/null
}

init_class1_server() {
  # Check whether it is fresh power on reset
  if [ "$(is_bmc_por)" -eq 1 ]; then
    # Disable clearing of PWM block on WDT SoC Reset
    devmem_clear_bit "$(scu_addr 70)" 13

    check_por_config 1
    if [ "$(is_sb_bic_ready 1)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
      power-util slot1 on
    fi

    check_por_config 2
    if [ "$(is_sb_bic_ready 2)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
      power-util slot2 on
    fi

    check_por_config 3
    if [ "$(is_sb_bic_ready 3)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
      power-util slot3 on
    fi

    check_por_config 4
    if [ "$(is_sb_bic_ready 4)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
      power-util slot4 on
    fi
  fi
}

/usr/local/bin/sync_date.sh

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_server
elif [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_server
else
  echo "Is board id correct(id=$bmc_location)?..."
fi
