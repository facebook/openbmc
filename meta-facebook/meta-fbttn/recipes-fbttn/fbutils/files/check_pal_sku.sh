#!/bin/sh
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

get_sku()
{
  # * SCC_RMT_TYPE_0  GPIOF7  47
  # 1: JBOD mode (Type 7); 0: dual server mode (Type5)
  SCC_RMT=`cat /sys/class/gpio/gpio47/value`
  get_locl
  SLOTID=$?
 
  PAL_SKU=$((($SCC_RMT << 2) | $SLOTID))
  return $PAL_SKU
}

get_locl()
{
  # * SLOTID_0  GPIOG0  48
  # * SLOTID_1  GPIOG1  49
  # SLOTID_[0:1]: 01=IOM_A; 10=IOM_B
  SLOTID_0=`cat /sys/class/gpio/gpio48/value`
  SLOTID_1=`cat /sys/class/gpio/gpio49/value`
  SLOTID=$((($SLOTID_1 << 1) | $SLOTID_0))

  return $SLOTID
}

get_sku
echo "Platform SKU: $?"
echo "<SKU[2:0] = {SCC_RMT_TYPE_0, SLOTID_1, SLOTID_0}>"
