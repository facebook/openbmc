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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

KERNEL_VERSION=$(uname -r)
SENSORS_CONF_PATH="/etc/sensors.d"
FCM_T_BOARD_REV=$(head -n1 "$TOP_FCMCPLD_SYSFS_DIR"/board_ver)
FCM_B_BOARD_REV=$(head -n1 "$BOTTOM_FCMCPLD_SYSFS_DIR"/board_ver)

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

# Config FCM Rsense value at different hardware version.
# Kernel driver use Rsense equal to 1 milliohm. We need to correct Rsense
# value, and all value are from hardware team.
if [ "$FCM_T_BOARD_REV" = "0x4" ] || [ "$FCM_T_BOARD_REV" = "0x5" ]; then
    # For R0D or R01 FCM
    fcm_t_rsense=2.651
elif [ "$FCM_T_BOARD_REV" = "0x7" ] || [ "$FCM_T_BOARD_REV" = "0x2" ]; then
    # For R0A, R0B or R0C FCM
    fcm_t_rsense=3.03
else
    # Default, keep Rsense to 1, if we have new FCM version, we can use default
    # Rsense value as a base to correct real value.
    fcm_t_rsense=1
fi

# Config FCM Rsense value at different hardware version.
# Kernel driver use Rsense equal to 1 milliohm. We need to correct Rsense
# value, and all value are from hardware team.
if [ "$FCM_B_BOARD_REV" = "0x4" ] || [ "$FCM_B_BOARD_REV" = "0x5" ]; then
    # For R0D or R01 FCM
    fcm_b_rsense=2.659
elif [ "$FCM_B_BOARD_REV" = "0x7" ] || [ "$FCM_B_BOARD_REV" = "0x2" ]; then
    # For R0A, R0B or R0C FCM
    fcm_b_rsense=3.03
else
    # Default, keep Rsense to 1, if we have new FCM version, we can use default
    # Rsense value as a base to correct real value.
    fcm_b_rsense=1
fi

sed "s/rsense/$fcm_t_rsense/g" "$SENSORS_CONF_PATH"/custom/adm1278-FCM-T.conf \
> "$SENSORS_CONF_PATH"/fcm-t.conf

sed "s/rsense/$fcm_b_rsense/g" "$SENSORS_CONF_PATH"/custom/adm1278-FCM-B.conf \
> "$SENSORS_CONF_PATH"/fcm-b.conf
