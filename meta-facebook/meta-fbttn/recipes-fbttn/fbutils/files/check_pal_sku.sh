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
  get_iom_type
  IOM_TYPE=$?
   if [ $SCC_RMT -eq 0 ]; then
    echo "System: Type5"
    if [ $SLOTID -eq 1 ]; then
      echo "Side: IOMA"
    else
      echo "Side: IOMB"
    fi
  else
    echo "System: Type7"
  fi
  PAL_SKU=$((($SCC_RMT << 6) | ($SLOTID << 4) | $IOM_TYPE))

  PAL_SKU=$((($SCC_RMT << 6) | ($SLOTID << 4) | $IOM_TYPE))
  return $PAL_SKU
}

get_locl()
{
  # * SLOTID_0  GPIOG0  48
  # * SLOTID_1  GPIOG1  49
  # SLOTID_[0:1]: 01=IOM_A; 10=IOM_B
  SLOTID_0=`cat /sys/class/gpio/gpio48/value`
  SLOTID_1=`cat /sys/class/gpio/gpio49/value`
  SLOTID=$((($SLOTID_0 << 1) | $SLOTID_1))

  return $SLOTID
}

get_iom_type()
{
  # * IOM_TYPE0  GPIOJ4  76
  # * IOM_TYPE1  GPIOJ5  77
  # * IOM_TYPE2  GPIOJ6  78
  # * IOM_TYPE3  GPIOJ7  79
  IOM_TYPE_0=`cat /sys/class/gpio/gpio76/value`
  IOM_TYPE_1=`cat /sys/class/gpio/gpio77/value`
  IOM_TYPE_2=`cat /sys/class/gpio/gpio78/value`
  IOM_TYPE_3=`cat /sys/class/gpio/gpio79/value`
  IOM_TYPE=$((($IOM_TYPE_0 << 3) | ($IOM_TYPE_1 << 2) | ($IOM_TYPE_2 << 1) | $IOM_TYPE_3))

  return $IOM_TYPE
}

get_sku
PAL_SKU=$?
echo -n "Platform SKU: $PAL_SKU ("
echo -n "obase=2;$PAL_SKU" | bc
echo ")"
echo "<SKU[6:0] = {SCC_RMT_TYPE_0, SLOTID_0, SLOTID_1, IOM_TYPE0, IOM_TYPE1, IOM_TYPE2, IOM_TYPE3}>"

exit $PAL_SKU
