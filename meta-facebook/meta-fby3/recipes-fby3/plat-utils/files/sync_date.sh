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
. /usr/local/fbpackages/utils/ast-functions

PATH=/sbin:/bin:/usr/sbin:/usr/bin

function get_sel_time_from_server {
  local BOARD_ID=$(get_bmc_board_id)
  local output=""
  local snum=0

  if [ $BOARD_ID -eq 9 ]; then
    output="$(/usr/local/bin/me-util slot1 0x28 0x48)"
    [ ${#output} == 12 ] && snum=1
  else
    for i in $(seq 1 4)
    do
      if [[ $(is_server_prsnt $i) == "1" ]] ; then
        # Use standard IPMI command 'get-sel-time' to read RTC time
        output=$(/usr/local/bin/me-util slot$i 0x28 0x48)
        if [ ${#output} == 12 ]; then
          snum=$i
          break
        fi
      fi
    done
  fi

  # return IPMI "Get sel time" command output, or return empty string if command failed
  [ ${#output} == 12 ] && echo $output || echo ""

  return $snum
}

function do_sync {
  local sel_time=$1
  date -s @$((16#$(echo "$sel_time" | awk '{print $4$3$2$1}')))
  test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
  echo 1 > /tmp/sync_date
}

# Sync BMC's date with one of the four servers
function sync_date {
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    if [ -f /tmp/cache_store/time_sync ] ; then
      # ND Server Time Sync by IPMI command
      output=$(cat /tmp/cache_store/time_sync)
      if [ ${#output} == 11 ] ; then
        echo Syncing up BMC time with server...
        do_sync "$output"
      fi
    else
      output=$(get_sel_time_from_server)
      num=$?
      if [ -n "$output" ]; then
        echo Syncing up BMC time with server$num...
        do_sync "$output"
      fi
    fi
  else
    test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
    echo 1 > /tmp/sync_date
  fi
}

sync_date
