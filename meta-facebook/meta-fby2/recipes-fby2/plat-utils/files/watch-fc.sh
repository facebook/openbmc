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

. /usr/local/fbpackages/utils/ast-functions

MAC1LINK=A0   # GPIOA0
FAB0_PRES=C4  # GPIOC4
FAB1_PRES=C5  # GPIOC5

while true; do
  # fabN_pres: active low.
  if [ $(gpio_get SMB_MEZZ_NIC_CLK_R $FAB0_PRES) = 0 ]; then
    gpio_set BMC_READY_N $MAC1LINK 0
  elif [ $(gpio_get SMB_MEZZ_NIC_DAT_R $FAB1_PRES) = 0 ]; then
    gpio_set BMC_READY_N $MAC1LINK 1
  fi
  sleep 1
done
