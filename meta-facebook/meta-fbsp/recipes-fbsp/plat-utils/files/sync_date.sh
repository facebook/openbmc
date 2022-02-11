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
MAX_RETRY_CNT=5

# Sync BMC's date with the server
sync_date()
{
  # if no ntp peer has been declared the system peer
  if ! /usr/sbin/ntpq -p | grep '^\*' > /dev/null ; then
    retry=0
    while [ $retry -lt $MAX_RETRY_CNT ]
    do
      # Use standard IPMI command 'get-sel-time' to read RTC time
      output=$(/usr/local/bin/me-util 0x28 0x48)
      if [ ${#output} == 12 ]; then
        ts=$(date -s @$((16#$(echo "$output" | awk '{print $4$3$2$1}'))))
        test -x /etc/init.d/hwclock.sh && /etc/init.d/hwclock.sh stop

        logger -s -p user.info -t sync_date "Syncing up BMC time with server..."
        logger -s -p user.info -t sync_date "Date: $ts"
        return
      else
        retry=$((retry+1))
        logger -s -p user.info -t sync_date "Get SEL time from ME failed, syncing up BMC time with server failed for $retry times."
        sleep 1
      fi
    done
  fi
}

sync_date
