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

# Sync BMC's date with one of the four servers
sync_date()
{
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    if [ -f /tmp/cache_store/time_sync ] ; then
      # ND Server Time Sync by IPMI command
      output=$(cat /tmp/cache_store/time_sync)
      if [ ${#output} == 11 ] ; then
        echo Syncing up BMC time with server...
        date -s @$((16#$(echo "$output" | awk '{print $4$3$2$1}')))
        test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
        echo 1 > /tmp/sync_date
      fi
    else
      for i in 1 2 3 4
      do
        if [[ $(is_server_prsnt $i) == "1" && $(get_slot_type $i) == "0" ]] ; then
          # Use standard IPMI command 'get-sel-time' to read RTC time
          s_type=$(get_server_type $i)
          if [ "$s_type" == "0" ] ; then
            output=$(/usr/local/bin/me-util slot$i 0x28 0x48)
          elif [ "$s_type" == "2" ] ; then
            output=$(/usr/bin/bic-util slot$i 0x28 0x48)
          fi
          # if the command fails, continue to next slot
          [ ${#output} != 12 ] && continue
          echo Syncing up BMC time with server$i...
          date -s @$((16#$(echo "$output" | awk '{print $4$3$2$1}')))
          test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
          echo 1 > /tmp/sync_date
          break
        fi
      done
    fi
  else
    test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
    echo 1 > /tmp/sync_date
  fi
}

sync_date
