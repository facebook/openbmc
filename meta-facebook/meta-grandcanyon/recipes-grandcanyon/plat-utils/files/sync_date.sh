#!/bin/sh
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

MAX_RETRY=15

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

##### sync time from Host
if [ "$(is_server_prsnt)" = "$SERVER_PRESENT" ] ; then
  retry=0
  while [ $retry -lt "$MAX_RETRY" ]
  do
    output=$("$ME_CMD" server "$NETFN_STORAGE_REQ" "$CMD_GET_SEL_TIME")
    
    # sync BMC time only when ME returns correct response length
    if [ "$(echo $output | wc -c)" = "$SEL_TIME_RESPONSE_LEN" ] ; then
      time_hex0=$(echo "$output" | cut -d' ' -f1 | sed 's/^0*//') # get byte0
      time_hex1=$(echo "$output" | cut -d' ' -f2 | sed 's/^0*//') # get byte1
      time_hex2=$(echo "$output" | cut -d' ' -f3 | sed 's/^0*//') # get byte2
      time_hex3=$(echo "$output" | cut -d' ' -f4 | sed 's/^0*//') # get byte3
      
      # change hexadecimal to decimal
      time_dec=$((0x$time_hex3 << 24 | 0x$time_hex2 << 16 | 0x$time_hex1 << 8 | 0x$time_hex0))
      
      # change decimal number to date formate
      sync_date=$(date -s @$time_dec +"%Y.%m.%d-%H:%M:%S")
      ret=$?
      if [ $ret = "$SYNC_TIME_OK" ]; then
        logger -s -p user.crit -t sync-date "Syncing up BMC time from Host(ME)"
        exit 0
      else
        # get invalid SEL time
        logger -s -p user.info -t sync-date "Syncing up BMC time from Host(ME) failed because get invalid SEL time: $output"
        retry=$((retry+1))
      fi
    else
      retry=$((retry+1))
    fi
  done
  logger -s -p user.info -t sync-date "Syncing up BMC time from Host(ME) failed because get SEL time from ME failed"
fi

##### sync time from default time
logger -s -p user.crit -t sync-date "Syncing up BMC time from Default(OpenBMC build time)"
