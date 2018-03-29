#!/bin/sh
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

### BEGIN INIT INFO
# Provides:          setup-crond
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup crond history buffering
### END INIT INFO

echo "Setup crond..."
  if [ ! -d "/etc/cron.daily" ]; then
      mkdir /etc/cron.daily
  fi

  if [ ! -d "/etc/cron.hourly" ]; then
      mkdir /etc/cron.hourly
  fi

  if [ ! -x /etc/cron.hourly/logrotate ]; then
      cp /etc/cron.daily/logrotate /etc/cron.hourly/
  fi

  mkdir -p /etc/cron/crontabs
  echo "0 * * * * run-parts /etc/cron.hourly" > /etc/cron/crontabs/root
  echo "0 0 * * * run-parts /etc/cron.daily" >> /etc/cron/crontabs/root
  /etc/init.d/busybox-cron start
echo "done."
