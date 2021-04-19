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
. /usr/local/fbpackages/utils/ast-functions
# Sync BMC's date with the server
sync_date()
{
  sleep 1
  # if no ntp peer has been declared the system peer
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    echo Syncing up BMC time with server...

    position=$(($(/usr/bin/kv get mb_pos)))
    host=$(($(/usr/bin/kv get mb_skt) & 0x1))  #0: Master 1:Slave

    if [ $position -eq 0 ]; then #position 0
      let addr=$((16#22))
    else
      let addr=$((16#20))
    fi

    # Use standard IPMI command 'get-sel-time' to read RTC time
    for i in {1..10};
    do
    #Master
      if [ $host -eq 0 ]; then
        output=$(/usr/local/bin/ipmb-util 5 0x2c 0x28 0x48)
        if [ ${#output} == 12 ]
        then
          break;
        fi

        if [ ${i} == 10 ]
        then
          exit
        fi
    #Slave
      else
        output=$(/usr/local/bin/ipmb-util 2 $addr 0xC0 0x34 0x01 0x0a 0x48)
        if [ ${#output} == 12 ]
        then
          break;
        fi

        if [ ${i} == 10 ]
        then
          echo Sync time with Master BMC
          output=$(/usr/local/bin/ipmb-util 2 $addr 0x28 0x48)
        fi
      fi
      usleep 300
    done

    sys_date=$(date +%s)
    set_date=$((16#$(echo "$output" | awk '{print $4$3$2$1}')))

    if [ $sys_date -ge $set_date ]; then
      echo "Use BMC date"
      let set_date=$sys_date
    fi

    date -s @$set_date
    test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop
  fi
}

sync_date
