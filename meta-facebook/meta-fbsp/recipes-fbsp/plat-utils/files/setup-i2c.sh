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

rev=$(cat /tmp/mb_rev)
sku=$(cat /tmp/mb_sku)

#REV[1:0]: 01(EVT), 10(DVT)
#EVT SKU_ID[2:1] = 00(INFINEON), 01(T1)
#DVT SKU_ID[2:1] = 00(TI), 01(INFINEON), TODO: 10(3rd Source)
if [ $rev -eq 1 ]; then
  sku=$((sku & 0x4))
  if [ $sku -eq 4 ]; then
    i2c_device_add 1 0x60 tps53688
    i2c_device_add 1 0x62 tps53688
    i2c_device_add 1 0x66 tps53688
    i2c_device_add 1 0x68 tps53688
    i2c_device_add 1 0x6c tps53688
    i2c_device_add 1 0x70 tps53688
    i2c_device_add 1 0x72 tps53688
    i2c_device_add 1 0x76 tps53688
  else
    i2c_device_add 1 0x60 xdpe12284
    i2c_device_add 1 0x62 xdpe12284
    i2c_device_add 1 0x66 xdpe12284
    i2c_device_add 1 0x68 xdpe12284
    i2c_device_add 1 0x6c xdpe12284
    i2c_device_add 1 0x70 xdpe12284
    i2c_device_add 1 0x72 xdpe12284
    i2c_device_add 1 0x76 xdpe12284
  fi
elif [ $rev -eq 2 ]; then
  sku=$((sku & 0x2))
  if [ $sku -eq 0 ]; then
    i2c_device_add 1 0x60 tps53688
    i2c_device_add 1 0x62 tps53688
    i2c_device_add 1 0x66 tps53688
    i2c_device_add 1 0x68 tps53688
    i2c_device_add 1 0x6c tps53688
    i2c_device_add 1 0x70 tps53688
    i2c_device_add 1 0x72 tps53688
    i2c_device_add 1 0x76 tps53688
  else
    i2c_device_add 1 0x60 xdpe12284
    i2c_device_add 1 0x62 xdpe12284
    i2c_device_add 1 0x66 xdpe12284
    i2c_device_add 1 0x68 xdpe12284
    i2c_device_add 1 0x6c xdpe12284
    i2c_device_add 1 0x70 xdpe12284
    i2c_device_add 1 0x72 xdpe12284
    i2c_device_add 1 0x76 xdpe12284
  fi
fi

if [ "$(gpio_get HP_LVC3_OCP_V3_1_PRSNT2_N)" == "0" ]; then
  i2c_device_add 17 0x50 24c32
else
  echo "Unknown or no device on the NIC#0"
fi

if [ "$(gpio_get HP_LVC3_OCP_V3_2_PRSNT2_N)" == "0" ]; then
  i2c_device_add 18 0x52 24c32
  ifconfig eth1 up
else
  echo "Unknown or no device on the NIC#1"
fi

if [ "$(gpio_get FM_SLOT1_PRSNT_N)" == "0" ]; then
  i2c_device_add 2 0x50 24c32
else
  echo "Unknown or no device on the RISER_SLOT#1"
fi

if [ "$(gpio_get FM_SLOT2_PRSNT_N)" == "0" ]; then
  i2c_device_add 6 0x50 24c32
else
  echo "Unknown or no device on the RISER_SLOT#2"
fi
