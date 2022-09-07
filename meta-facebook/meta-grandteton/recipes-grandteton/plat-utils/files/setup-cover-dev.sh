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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

rebind_i2c_dev() {
  dev="$1-00$2"
  dri=$3

  if [ ! -L "${SYSFS_I2C_DEVICES}/$dev/driver" ]; then
    if i2c_bind_driver "$dri" "$dev" >/dev/null; then
      echo "rebind $dev to driver $dri successfully"
    fi
  fi
}

vr_sku=$(($(/usr/bin/kv get mb_sku) & 0x03))  #0x00 : RAA,
                                              #0x01 : INF,
                                              #0x10 : MPS,

echo "Probe VR Device"
#CPU VR

if [ "$vr_sku" -eq 2 ]; then
  i2c_device_add 20 0x60 mp2971
  i2c_device_add 20 0x61 mp2971
  i2c_device_add 20 0x63 mp2971
  i2c_device_add 20 0x72 mp2971
  i2c_device_add 20 0x74 mp2971
  i2c_device_add 20 0x76 mp2971
elif [ "$vr_sku" -ge 1 ]; then
  i2c_device_add 20 0x60 xdpe152c4
  i2c_device_add 20 0x62 xdpe152c4
  i2c_device_add 20 0x64 xdpe152c4
  i2c_device_add 20 0x72 xdpe152c4
  i2c_device_add 20 0x74 xdpe152c4
  i2c_device_add 20 0x76 xdpe152c4
else
  i2c_device_add 20 0x60 isl69260
  i2c_device_add 20 0x61 isl69260
  i2c_device_add 20 0x63 isl69260
  i2c_device_add 20 0x72 isl69260
  i2c_device_add 20 0x74 isl69260
  i2c_device_add 20 0x76 isl69260
fi

HSC_MAIN_SOURCE="0"

#MB HSC
if [ "$(gpio_get FM_BOARD_BMC_SKU_ID2)" -eq "$HSC_MAIN_SOURCE" ]; then
  i2c_device_add 2 0x20 mp5990
else
  i2c_device_add 21 0x4C lm75
  i2c_device_add 2 0x41 ltc4282
  i2c_device_add 2 0x51 24c64
fi

