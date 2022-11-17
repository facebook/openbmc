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

mb_sku=$(($(/usr/bin/kv get mb_sku) & 0x0F))
# sku_id[3:0] | HSC     | ADC        | VR             | config
# 0000        | MPS5990 | ADC128D818 | RAA229620      |   0
# 0100        | LTC4282 | MAXIM11617 | XDPE192C3B     |   1
# 0010        | TBD     | ADC128D818 | RAA229620      |   2
# 0110        | LTC4286 | MAXIM11617 | MP2857GQKT-001 |   3
# 0001        | MPS5990 | ADC128D818 | MP2857GQKT-001 |   4

config0="0"
config1="4"

#TBD
#config2="2"
#config3="6"
#config4="1"

MB_1ST_SOURCE="0"
MB_2ND_SOURCE="1"
MB_3RD_SOURCE="2"

#HSC
probe_hsc_mp5990() {
  i2c_device_add 6 0x20 mp5990
  kv set mb_hsc_source "$MB_1ST_SOURCE"
}

probe_hsc_ltc() {
  val=$(i2cget -f -y 21 0x42 0x02)
  data=$(("$val"))

  #Read ADC 1.5v:LTC4286 1v:LTC4282
  if [ "$data" -gt 4 ]; then
    i2c_device_add 6 0x41 ltc4286
    kv set mb_hsc_source "$MB_3RD_SOURCE"
  else
    i2c_device_add 6 0x41 ltc4282
    kv set mb_hsc_source "$MB_2ND_SOURCE"
  fi

  #HSC Temp
  i2c_device_add 25 0x4C lm75
  #HSC module's eeprom
  i2c_device_add 6 0x51 24c64
}

#ADC
probe_adc_ti() {
  i2cset -f -y 20 0x1d 0x0b 0x02
  i2c_device_add 20 0x1d adc128d818
}

probe_adc_maxim() {
  i2c_device_add 20 0x35 max11617
}

#VR
probe_vr_xdpe() {
   i2c_device_add 20 0x4a xdpe152c4
   i2c_device_add 20 0x49 xdpe152c4
   i2c_device_add 20 0x4c xdpe152c4
   i2c_device_add 20 0x4d xdpe152c4
   i2c_device_add 20 0x4e xdpe152c4
   i2c_device_add 20 0x4f xdpe152c4
}

probe_vr_raa() {
  i2c_device_add 20 0x61 isl69260
  i2c_device_add 20 0x62 isl69260
  i2c_device_add 20 0x63 isl69260
  i2c_device_add 20 0x72 isl69260
  i2c_device_add 20 0x74 isl69260
  i2c_device_add 20 0x75 isl69260
}

if [ "$mb_sku" -eq "$config0" ]; then
  probe_hsc_mp5990
  probe_adc_ti
  probe_vr_raa
elif [ "$mb_sku" -eq "$config1" ]; then
  probe_hsc_ltc
  probe_adc_maxim
  probe_vr_xdpe
fi
