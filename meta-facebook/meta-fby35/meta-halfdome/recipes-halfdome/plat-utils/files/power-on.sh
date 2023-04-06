#!/bin/bash
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
DEBUG_ENABLE=false


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

update_ntp_config_from_kv() {
  local ntp_server
  ntp_server=$(/usr/bin/kv get ntp_server persistent)
  if [ -z "$ntp_server" ]; then
    return
  fi

  sed -i "s/.*# REPLACE WITH KV.*/server $ntp_server iburst # REPLACE WITH KV/" /etc/ntp.conf
  /etc/init.d/ntpd restart
}

is_ntp_time_synced() {
  local str
  local ret

  str=$(/usr/sbin/ntpq -np | grep '^\*')
  ret=$?

  if [ "$DEBUG_ENABLE" = true ] ; then
    echo "=================================="
    echo "ret: $ret"
    echo "----------------------------------"
    echo "$str"
    echo "=================================="
  fi

  # ret == 0, synced
  # ret == 1, not synced
  return $ret
}

ntp_sync_delay() {
    local NTP_DELAY_COUNT

    NTP_DELAY_COUNT=0
    echo "[$(/bin/date)] ntp is enabled, ntp_server: $(/usr/bin/kv get ntp_server persistent)"
    while [ $NTP_DELAY_COUNT -lt 30 ];
    do
      if is_ntp_time_synced ; then
        break
      fi
      ((NTP_DELAY_COUNT++))
      [ "$DEBUG_ENABLE" = true ] && echo "[$(/bin/date)] system time not synced, count: ${NTP_DELAY_COUNT}"
      sleep 1
    done
    if is_ntp_time_synced ; then
      echo "[$(/bin/date)] system time synced after $NTP_DELAY_COUNT sec"
    else
      echo "[$(/bin/date)] system time not synced after $NTP_DELAY_COUNT sec"
    fi
}

# Check whether it is fresh power on reset
if [ "$(is_bmc_por)" -eq 1 ]; then
  # Disable clearing of PWM block on WDT SoC Reset
  devmem_clear_bit "$(scu_addr 70)" 13

  # Wait for NTP sync
  update_ntp_config_from_kv
  ntp_sync_delay

  # check POR config for each slot
  check_por_config 1
  if [ "$(is_sb_bic_ready 1)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
    power-util slot1 on
  fi

  check_por_config 3
  if [ "$(is_sb_bic_ready 3)" = "1" ] && [ $TO_PWR_ON -eq 1 ]; then
    power-util slot3 on
  fi

fi
