#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

### BEGIN INIT INFO
# Provides:          setup-ntp
# Required-Start:
# Required-Stop:
# Default-Start:     5
# Default-Stop:
# Short-Description: Update NTP config from kv store and perform one-shot time sync
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

NTP_SERVER=$($KV_CMD get ntp_server persistent)
if [ -n "$NTP_SERVER" ]; then
  # Remove cfg-util residual entries (appended without # REPLACE WITH KV marker)
  sed -i "/^server $NTP_SERVER iburst$/d" /etc/ntp.conf
  sed -i "/^restrict $NTP_SERVER$/d" /etc/ntp.conf
  # Update placeholder line with configured NTP server
  sed -i "s/.*# REPLACE WITH KV.*/server $NTP_SERVER iburst # REPLACE WITH KV/" /etc/ntp.conf
  logger -s -p user.info -t setup-ntp "NTP server configured: $NTP_SERVER"

  # Background: wait for network, one-shot sync, restart ntpd
  (
    # Wait for eth0 to get an IP (max 30s)
    i=0
    while [ "$i" -lt 30 ]; do
      IP=$(ip -4 addr show eth0 2>/dev/null | grep -o 'inet [0-9.]*' | awk '{print $2}')
      if [ -n "$IP" ]; then
        break
      fi
      sleep 1
      i=$((i + 1))
    done

    if [ -z "$IP" ]; then
      logger -s -p user.warn -t setup-ntp "No IP on eth0 after 30s, skipping NTP sync"
      exit 1
    fi

    # One-shot time sync (immediate clock jump)
    if sntp -S "$NTP_SERVER" > /dev/null 2>&1;
    then
      logger -s -p user.info -t setup-ntp "One-shot sync success from $NTP_SERVER (eth0=$IP)"
    else
      logger -s -p user.warn -t setup-ntp "One-shot sync failed from $NTP_SERVER (eth0=$IP)"
    fi

    # Restart ntpd with updated config
    sv restart ntpd 2>/dev/null || /etc/init.d/ntpd restart 2>/dev/null
    logger -s -p user.info -t setup-ntp "ntpd restarted with updated config"
  ) &
fi
