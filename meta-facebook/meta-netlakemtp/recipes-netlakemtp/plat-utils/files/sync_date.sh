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

MAX_RETRY=5
NTP_CMD=/usr/sbin/ntpq

##### sync time from NTP server
retry=0
while [ $retry -lt "$MAX_RETRY" ]
do
  if ! $NTP_CMD -p | grep '^\*' > /dev/null ; then
    retry=$((retry+1))
    sleep 1
  else
    logger -s -p user.crit -t sync-date "Syncing up BMC time from NTP"
    exit 0
  fi
done
logger -s -p user.info -t sync-date "Syncing up BMC time from NTP failed because NTP server not exist"

echo Syncing up BMC time with server...

# Use standard IPMI command 'get-sel-time' to read RTC time
date -s @$((16#$(/usr/local/bin/me-util 0x28 0x48 | awk '{print $4$3$2$1}')))
ret=$?
if [ $ret = "0" ]; then
    logger -s -p user.crit -t sync-date "Syncing up BMC time from Host(ME)"
    exit 0
fi

##### sync time from default time
logger -s -p user.crit -t sync-date "Syncing up BMC time from Default(OpenBMC build time)"
