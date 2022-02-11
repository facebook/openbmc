#!/bin/sh
#
# Copyright 2017-present Facebook. All Rights Reserved.
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
# Provides:          fw_env_config
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Replace /etc/fw_env.config by /etc/fw_env.config.full
### END INIT INFO

ACTION="$1"
case "$ACTION" in
  start)
    CFG_FULL=`grep "Partition_000" /proc/mtd`
    if [ "$CFG_FULL" != "" ]; then
      cp /etc/fw_env.config.full /etc/fw_env.config
    fi
    ;;
esac

exit 0
