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

sec_offset=0x0
min_offest=0x2
hour_offset=0x4
day_offset=0x7
mon_offset=0x8
year_offset=0x9
rtc_offset_seq=("$sec_offset" "$min_offest" "$hour_offset" "$day_offset" "$mon_offset" "$year_offset")

function set_sel_time_from_rtc {

  for i in $(seq 1 4)
  do
    if [[ $(is_sb_bic_ready "$i") == "1" ]] ; then
      setting=()
      for offset in "${rtc_offset_seq[@]}"
      do
        /usr/bin/bic-util "slot$i" 0x18 0x52 0x7 0xde 0x1 $offset
        if [ $? -ne 0 ]; then
          logger -t "sync_date" -p daemon.crit "Fail to get RTC register value, offset: $offset"
          break
        fi
        result=$(/usr/bin/bic-util "slot$i" 0x18 0x52 0x7 0xde 0x1 $offset | sed 's/ //g')

        setting+=($result)
      done

      date -s "20${setting[5]}-${setting[4]}-${setting[3]} ${setting[2]}:${setting[1]}:${setting[0]}"
      if [ $? -eq 0 ]; then
        test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
        kv set date_sync 1
        break
      else
        continue
      fi
    fi
  done
}

function get_sel_time_from_server {
  local BOARD_ID=$(get_bmc_board_id)
  local output=""
  local snum=0

  if [ "$BOARD_ID" -eq "$BMC_ID_CLASS2" ]; then
    output="$(/usr/local/bin/me-util slot1 0x28 0x48)"
    [ ${#output} == 12 ] && snum=1
  else
    for i in $(seq 1 4)
    do
      if [[ $(is_server_prsnt "$i") == "1" ]] ; then
        # Use standard IPMI command 'get-sel-time' to read RTC time
        output=$(/usr/local/bin/me-util "slot$i" 0x28 0x48)
        if [ ${#output} == 12 ]; then
          snum=$i
          break
        fi
      fi
    done
  fi

  # return IPMI "Get sel time" command output, or return empty string if command failed
  [ ${#output} == 12 ] && echo "$output" || echo ""

  return "$snum"
}

function do_sync {
  local sel_time=$1
  date -s @$((16#$(echo "$sel_time" | awk '{print $4$3$2$1}')))
  test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
  kv set date_sync 1
}

# Sync BMC's date with one of the four servers
function sync_date {
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    output="$(/usr/bin/kv get time_sync)"
    if [ -n "${output}" ] ; then
      # Server Time Sync by IPMI command
      if [ ${#output} == 11 ] ; then
        echo Syncing up BMC time with server...
        do_sync "$output"
      fi
    else
      sys_config=$(/usr/local/bin/show_sys_config | grep -i "config:" | awk -F ": " '{print $3}')
      server_type=$(get_server_type 1)
      if [ "$sys_config" = "B" ] && [[ $server_type -eq 4 ]]; then
        set_sel_time_from_rtc
      else
        output=$(get_sel_time_from_server)
        num=$?
        if [ -n "$output" ]; then
          echo Syncing up BMC time with server$num...
          do_sync "$output"
        fi
      fi
    fi
  else
    test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
    kv set date_sync 1
  fi
}

sync_date
