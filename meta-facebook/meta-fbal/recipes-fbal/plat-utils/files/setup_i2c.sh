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

sku=$(kv get mb_sku)
rev=$(kv get mb_rev)

# retry to probe pxe1110c (pxe1211c).
# note: still keep pxe1110c in dts, so it's the
# 1st device to be probed on i2c-1
if [ ! -L "${SYSFS_I2C_DEVICES}/1-004a/driver" ]; then
  i2c_bind_driver pxe1211c 1-004a 4 2>/dev/null
  [ $? -eq 0 ] && echo "rebind 1-004a to driver pxe1211c successfully"
fi

#DVT SKU_ID[2:1] = 00(TI), 01(INFINEON), TODO: 10(3rd Source)
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

if [ $rev -lt 2 ]; then
  i2c_device_add 4 0x54 24c64
else
  i2c_device_add 4 0x57 24c64
fi


if [ "$(gpio_get HP_LVC3_OCP_V3_1_PRSNT2_N)" == "0" ]; then
  i2c_device_add 17 0x50 24c32
fi

if [ "$(gpio_get HP_LVC3_OCP_V3_2_PRSNT2_N)" == "0" ]; then
  i2c_device_add 18 0x52 24c32
  if [ "$(kv get eth1_disable_ipv6 persistent 2>/dev/null)" != "0" ]; then
    echo 1 > /proc/sys/net/ipv6/conf/eth1/disable_ipv6
  fi
  ifconfig eth1 up
fi
