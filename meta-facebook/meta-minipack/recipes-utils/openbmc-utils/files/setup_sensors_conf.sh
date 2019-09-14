#!/bin/bash
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
# Provides:          setup_sensors_conf.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description: Setup lm-sensors config
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KERNEL_VERSION=$(uname -r)
SENSORS_CONF_PATH="/etc/sensors.d"

# In kernel version 4.1.x -
# pmbus_core.c use milliwatt for direct format power output,
# so we need to multiply power output by 1000.
# In higher kernel version -
# pmbus_core.c use microwatt for direct format power output,
# so we need to multiply power output by 1.
if [[ "$KERNEL_VERSION" != 4.1.* ]]; then
    div=1
else
    div=1000
fi

sed -i "s/power_factor/$div/g" "$SENSORS_CONF_PATH"/custom/*.conf
cp "$SENSORS_CONF_PATH"/custom/adm1278-SCM.conf "$SENSORS_CONF_PATH"/scm.conf
cp "$SENSORS_CONF_PATH"/custom/adm1278-FCM-T.conf "$SENSORS_CONF_PATH"/fcm-t.conf
cp "$SENSORS_CONF_PATH"/custom/adm1278-FCM-B.conf "$SENSORS_CONF_PATH"/fcm-b.conf
