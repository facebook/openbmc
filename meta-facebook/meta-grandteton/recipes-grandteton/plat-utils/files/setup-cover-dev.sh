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


MB_1ST_SOURCE="0"
MB_2ND_SOURCE="1"
MB_3RD_SOURCE="2"
MB_4TH_SOURCE="3"

MB_VR_SECOND="1"
MB_VR_THIRD="2"

MB_ADC_MAIN="0"
#MB ADM128D
if [ "$(gpio_get FM_BOARD_BMC_SKU_ID1)" -eq "$MB_ADC_MAIN" ]; then
  i2cset -f -y 20 0x1d 0x0b 0x02
  i2c_device_add 20 0x1d adc128d818
  kv set mb_adc_source "$MB_1ST_SOURCE"
else
  i2c_device_add 20 0x35 max11617
  kv set mb_adc_source "$MB_2ND_SOURCE"
fi

# MB CPU VR Device"
if [ "$vr_sku" -eq "$MB_VR_THIRD" ]; then
  i2c_device_add 20 0x60 mp2971
  i2c_device_add 20 0x61 mp2971
  i2c_device_add 20 0x63 mp2971
  i2c_device_add 20 0x72 mp2971
  i2c_device_add 20 0x74 mp2971
  i2c_device_add 20 0x76 mp2971
  kv set mb_vr_source "$MB_3RD_SOURCE"
elif [ "$vr_sku" -eq "$MB_VR_SECOND"  ]; then
  i2c_device_add 20 0x60 xdpe152c4
  i2c_device_add 20 0x62 xdpe152c4
  i2c_device_add 20 0x64 xdpe152c4
  i2c_device_add 20 0x72 xdpe152c4
  i2c_device_add 20 0x74 xdpe152c4
  i2c_device_add 20 0x76 xdpe152c4
  kv set mb_vr_source "$MB_2ND_SOURCE"
else
  i2c_device_add 20 0x60 isl69260
  i2c_device_add 20 0x61 isl69260
  i2c_device_add 20 0x63 isl69260
  i2c_device_add 20 0x72 isl69260
  i2c_device_add 20 0x74 isl69260
  i2c_device_add 20 0x76 isl69260
  kv set mb_vr_source "$MB_1ST_SOURCE"
fi

# MB HSC Device"
mb_hsc=$(($(gpio_get FM_BOARD_BMC_SKU_ID3) << 1 |
          $(gpio_get FM_BOARD_BMC_SKU_ID2)))

#MB_HSC_MAIN="0"   # mp5990
MB_HSC_SECOND="1"  # ltc4282/ltc4286
MB_HSC_THIRD="2"   # rs31380
mbrev=$(kv get mb_rev)
MB_DVT_BORAD_ID="3"


#kv set mb_hsc_module "0"
if [ "$mb_hsc" -eq "$MB_HSC_THIRD" ]; then
  kv set mb_hsc_source "$MB_4TH_SOURCE"
elif [ "$mb_hsc" -eq "$MB_HSC_SECOND" ]; then
  i2c_device_add 21 0x4C lm75
  i2c_device_add 2 0x51 24c64
  #Read ADC 1.5v:LTC4286 1v:LTC4282
  #DVT ADC INA230; EVT ADC ISL28022
  val=$(i2cget -f -y 21 0x42 0x02)
  data=$(("$val"))

  if [ "$mbrev" -ge "$MB_DVT_BORAD_ID" ]
  then
    if [ "$data" -ge 4 ]
    then
      i2c_device_add 2 0x41 ltc4286
      kv set mb_hsc_source "$MB_3RD_SOURCE"
    else
      i2c_device_add 2 0x41 ltc4282
      kv set mb_hsc_source "$MB_2ND_SOURCE"
    fi
  else
    if [ "$data" -gt 4 ]
    then
      i2c_device_add 2 0x41 ltc4286
      kv set mb_hsc_source "$MB_3RD_SOURCE"
    else
      i2c_device_add 2 0x41 ltc4282
      kv set mb_hsc_source "$MB_2ND_SOURCE"
    fi
  fi
  kv set mb_hsc_module "1"
else
  i2c_device_add 2 0x20 mp5990
  kv set mb_hsc_source "$MB_1ST_SOURCE"
fi

#MB DPM
mbrev=$(kv get mb_rev)
MB_DVT_BOARD_ID="3"

if [ "$mbrev" -ge "$MB_DVT_BOARD_ID" ]; then
  i2c_device_add 34 0x41 ina230
  i2c_device_add 34 0x42 ina230
  i2c_device_add 34 0x43 ina230
  i2c_device_add 34 0x44 ina230
  i2c_device_add 34 0x45 ina230
  kv set mb_dpm_source "$MB_1ST_SOURCE"
fi