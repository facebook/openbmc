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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

. /usr/local/bin/openbmc-utils.sh

if [ "$(gpio_get OCP_V3_0_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 1 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_4_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 9 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_1_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 2 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_5_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 10 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_2_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 4 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_6_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 11 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_3_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 7 0x50 24c32
fi

if [ "$(gpio_get OCP_V3_7_PRSNTB_R_N)" == "0" ]; then
  i2c_device_add 13 0x50 24c32
fi