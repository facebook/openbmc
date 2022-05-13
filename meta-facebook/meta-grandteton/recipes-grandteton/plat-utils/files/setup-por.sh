#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# Provides:          setup-por
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

  if POR=$(kv get "server_por_cfg" persistent); then
    if [ "$POR" = "lps" ]; then
      if LS=$(kv get "pwr_server_last_state" persistent); then
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

mb_server_power_on() {
  echo $TO_PWR_ON
  if [ "$(is_bmc_por)" -eq 1 ]; then
    check_por_config
    if [ $TO_PWR_ON -eq 1 ]; then
      power-util mb on
    fi
  fi
}

mb_server_power_on
