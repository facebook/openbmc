#!/bin/bash
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston

. /usr/local/fbpackages/utils/ast-functions

rev=$(kv get mb_rev)

if [ $rev -eq 0 ]; then
  gp_carrier1_prsnt=$(gpio_get NVME_0_1_PRSNTB_R_N)
  if [ $gp_carrier1_prsnt -eq 0 ]; then
    i2cset -y -f 21 0x77 0x6 0
  fi

  gp_carrier2_prsnt=$(gpio_get NVME_2_3_PRSNTB_R_N)
  if [ $gp_carrier2_prsnt -eq 0 ]; then
    i2cset -y -f 22 0x77 0x6 0
  fi
fi
