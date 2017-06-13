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
  # 1: single server mode (Type 7 Headnode) ; 0: dual server mode (Type5)
  SCC_RMT=`cat /sys/class/gpio/gpio47/value`
  get_iom_location
  SLOTID=$?
  get_iom_type
  IOM_TYPE=$?

  if [ $IOM_TYPE -eq 1 ]; then
    echo "System: Type5"
    if [ $SLOTID -eq 1 ]; then
      echo "IOM Location: Side A"
    elif [ $SLOTID -eq 2 ]; then
      echo "IOM Location: Side B"
    else
      echo "IOM Location: Unknown"
    fi
  elif [ $IOM_TYPE -eq 2 ]; then
    echo "System: Type7"
  else
    echo "System: Unknown"
  fi

  PAL_SKU=$((($SCC_RMT << 6) | ($SLOTID << 4) | $IOM_TYPE))

  return $PAL_SKU
}

get_iom_location()
{
  # * SLOTID_0  GPIOG0  48 Strap pin for IOM location
  # * SLOTID_1  GPIOG1  49 Strap pin for IOM location
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
i=0
while [ "${i}" -lt 8 ]
do
  tmp_sku_bit=$((($PAL_SKU >> i) & 1))
  sku_bit=$tmp_sku_bit$sku_bit
  i=$(($i+1))
done
  echo -n $sku_bit
echo ")"
echo "<SKU[6:0] = {SCC_RMT_TYPE_0, IOM_SLOTID0, IOM_SLOTID1, IOM_TYPE0, IOM_TYPE1, IOM_TYPE2, IOM_TYPE3}>"

exit $PAL_SKU
