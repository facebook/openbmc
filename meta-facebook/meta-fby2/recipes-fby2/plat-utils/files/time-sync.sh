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
# Provides:          time-sync
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Time Sync With Server
### END INIT INFO
. /usr/local/fbpackages/utils/ast-functions

time_sync_success=0
time_sync_with_IMC=0

# Sync BMC's date with one of the four servers
sync_date()
{
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    sleep 60    #wait for BIOS writes RTC time to internal memory 
    max_retry=6
    default_time_h=5A   #default time high byte 2018-01-01
    default_time_l=4A   #default time low byte 2018-01-01
    for i in 1 2 3 4 
    do
      if [[ $(is_server_prsnt $i) == "1" && $(get_slot_type $i) == "0" && $(get_server_type $i) == "1" ]] ; then
        retry_times=0
        time_sync_with_IMC=1
        while [ ${retry_times} -lt ${max_retry} ]
        do
          # Use IMC command 'get-rtc-time' to read RTC time
          output=$(/usr/bin/bic-util slot$i 0xe0 0x02 0x15 0xa0 0x00 0x10 0xb8 0xe3 0xa9 0x05 0x0 0x0 0x0 0x0 0x0)
          # if the command fails and the number of retry times reach max retry, continue to next slot
          if [ $(echo $output | wc -c) == 36 ] ; then 
            time_h=$((16#$(echo "$output" | cut -c 34-35)))
            time_l=$((16#$(echo "$output" | cut -c 31-32)))

            if [ "$time_h" -gt $((16#${default_time_h})) ] ; then
              echo Syncing up BMC time with server$i...
              date -s @$((16#$(echo $output | awk '{print $12$11$10$9}')))
              test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
              time_sync_success=1
              break
            fi

            if [[ "$time_h" -ge $((16#${default_time_h})) && "$time_l" -ge $((16#${default_time_l})) ]] ; then
              echo Syncing up BMC time with server$i...
              date -s @$((16#$(echo $output | awk '{print $12$11$10$9}')))
              test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
              time_sync_success=1
              break
            fi
          fi
          retry_times=$(($retry_times+1))
          sleep 5
        done
      fi
      if [ $time_sync_success == 1 ] ; then
        break
      fi
    done
    # This log only appears in the RC system
    if [[ $time_sync_success == 0 && $time_sync_with_IMC == 1 ]] ; then
      logger -p user.crit "Time sync with IMC failed, using 2018/01/01 as default" 
    fi
  fi
  sts=$(ifconfig eth0 | grep -i "inet addr")
  if [ "$sts" == "" ]; then    #No ipv4 ip
    kill -9 `cat /var/run/dhclient.eth0.pid`
    #if dhcleint need to restart, make it non-demonized so that dhclient will continuely re-access ipv4 address
    dhclient -d -pf /var/run/dhclient.eth0.pid eth0 > /dev/null 2>&1 &
  fi
}

sync_date

