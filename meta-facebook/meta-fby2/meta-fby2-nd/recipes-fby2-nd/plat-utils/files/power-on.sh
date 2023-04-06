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

DEF_PWR_ON=1
TO_PWR_ON=
POWER_ON_SLOT=
RETRY=0

DEBUG_ENABLE=false

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

check_por_config()
{
  TO_PWR_ON=-1
  local PWR_ON_RST
  local LAST_STATE

  PWR_ON_RST=$(/usr/bin/kv get slot${1}_por_cfg persistent)

  if [ -z "${PWR_ON_RST}" ]; then       # File not exist
    TO_PWR_ON=$DEF_PWR_ON
  elif [ "$PWR_ON_RST" == "on" ]; then    # Case ON
    TO_PWR_ON=1;
  elif [ "$PWR_ON_RST" == "off" ]; then   # Case OFF
    TO_PWR_ON=0;
  elif [ "$PWR_ON_RST" == "lps" ]; then   # Case LPS
    LAST_STATE=$(/usr/bin/kv get pwr_server${1}_last_state persistent)
    if [ -z "${LAST_STATE}" ]; then
      TO_PWR_ON=$DEF_PWR_ON
    elif [ "$LAST_STATE" == "on" ]; then
      TO_PWR_ON=1;
    elif [ "$LAST_STATE" == "off" ]; then
      TO_PWR_ON=0;
    else
      echo "unexpected value in pwr_server${1}_last_state"
    fi
  else
    echo "unexpected value in slot${1}_por_cfg"
  fi
}

remove_gp_m2_info() {
  for i in {2..13};
  do
    /usr/bin/kv del sys_config/fru2_m2_${i}_info persistent > /dev/null 2>&1
    /usr/bin/kv del sys_config/fru4_m2_${i}_info persistent > /dev/null 2>&1
  done
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
if [ $(is_bmc_por) -eq 1 ]; then
  # Remove all GP/GPv2 info
  remove_gp_m2_info

  # Wait for NTP sync
  update_ntp_config_from_kv
  ntp_sync_delay

  # check POR config for each slot
  for i in $(seq 1 4)
  do
    check_por_config ${i}
    if [ $TO_PWR_ON -eq 1 ] && [ $(is_server_prsnt ${i}) == "1" ] && [ $(get_slot_type ${i}) == "0" ] ; then
      POWER_ON_SLOT+=(${i})
      power-util slot${i} on &
    fi
  done

  # Wait for all slots to finish power-up.
  wait

fi
