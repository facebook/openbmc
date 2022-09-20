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
#

PATH=/sbin:/bin:/usr/sbin:/usr/bin

# shellcheck disable=SC1091
. /usr/local/fbpackages/utils/ast-functions

DEBUG_ENABLE=false

is_ntp_enabled() {
  local ntp_server
  ntp_server=$(/usr/bin/kv get ntp_server persistent)
  if ! /usr/sbin/ntpdate -q "$ntp_server" > /dev/null ; then
    return 1 # disabled
  fi
  return 0 # enabled
}

is_ntp_time_synced() {
  local str
  local ret

  str=$(/usr/sbin/ntpq -np | grep '^\*')
  ret=$?
  # logger -t "ipmid" -p daemon.crit "ret: $ret"
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

    while [ $NTP_DELAY_COUNT -lt 60 ];
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

sync_date()
{
  if [ "$(is_bmc_por)" -eq 1 ]; then

    sleep 5
    if is_ntp_enabled ; then
      ntp_sync_delay
    fi
    echo Syncing up BMC time with server...
  fi
}

sync_date
