#!/bin/bash
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

SMBCPLD=/sys/bus/i2c/devices/i2c-12/12-003e

if [ $# -ne 1 ]; then
    exit -1
fi

img="$1"

source /usr/local/bin/openbmc-utils.sh

top_fan_cpld="${SMBCPLD}/smb_cpld_fcm_b_cpld_sel"
bottom_fan_cpld="${SMBCPLD}/smb_cpld_fcm_b_cpld_sel"

trap 'gpio_set BMC_FCM_T_MUX_SEL 0; \
      gpio_set BMC_FCM_B_MUX_SEL 0; \
      echo 0 > "${top_fan_cpld}"; \
      echo 0 > "${bottom_fan_cpld}"; \
      rm -rf /tmp/fcmcpld_update' INT TERM QUIT EXIT

# change BMC_FCM_T_MUX_SEL and smb_cpld_fcm_t_cpld_sel to 1
# to connect BMC to Top FANCPLD pins
gpio_set BMC_FCM_T_MUX_SEL 1
echo 1 > "${top_fan_cpld}"

# change BMC_FCM_B_MUX_SEL and smb_cpld_fcm_t_cpld_sel to 1
# to connect BMC to Bottom FANCPLD pins
gpio_set BMC_FCM_B_MUX_SEL 1
echo 1 > "${bottom_fan_cpld}"

echo 1 > /tmp/fcmcpld_update

ispvm dll /usr/lib/libcpldupdate_dll_gpio.so "${img}" \
    --tms BMC_FCM_CPLD_TMS \
    --tdo BMC_FCM_CPLD_TDI \
    --tdi BMC_FCM_CPLD_TDO \
    --tck BMC_FCM_CPLD_TCK
result=$?
# 1 is returned upon upgrade success
if [ $result -eq 1 ]; then
    echo "Upgrade successful."
    exit 0
else
    echo "Upgrade failure. Return code from utility : $result"
    exit 1
fi
