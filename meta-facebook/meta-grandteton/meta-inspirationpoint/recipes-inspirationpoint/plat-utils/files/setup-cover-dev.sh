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
# sku_id[3:0] | HSC     | ADC        | VR             | RT VR        | DPM        | config
# 0000        | MPS5990 | ADC128D818 | RAA229620      | ISL69260IRAZ |            |  0
# 0100        | LTC4282 | MAXIM11617 | XDPE192C3B     | XDPE15284D   |            |  1
# 0010        | TBD     | ADC128D818 | RAA229620      | TBD          |            |  2
# 0110        | LTC4286 | MAXIM11617 | MP2857GQKT-001 | TBD          |            |  3
# 0001        | MPS5990 | ADC128D818 | MP2857GQKT-001 | ISL69260IRAZ |            |  4

# Artemis
# 1000        | LTC4282 | ADC128D818 | RAA229620      | ISL69260IRAZ | ISL28022   |  8
# 1001        | LTC4282 | ADC128D818 | XDPE192C3B     | XDPE15284D   | ISL28022   |  9
# 1011        | LTC4282 | MAXIM11617 | RAA229620      | ISL69260IRAZ | INA230     |  11
# 1100        | MPS5990 | ADC128D818 | XDPE192C3B     | XDPE15284D   | ISL28022   |  12

config0="0"
config1="4"
config4="1"

# Artemis
config8="8"
config9="9"
config11="11"
config12="12"

#TBD
#config2="2"
#config3="6"


MB_1ST_SOURCE="0"
MB_2ND_SOURCE="1"
MB_3RD_SOURCE="2"

mbrev=$(kv get mb_rev)
MB_DVT_BORAD_ID="2"

#HSC
probe_hsc_mp5990() {
  i2c_device_add 6 0x20 mp5990
  kv set mb_hsc_source "$MB_1ST_SOURCE"
}

probe_hsc_ltc() {
  val=$(i2cget -f -y 21 0x42 0x02)
  data=$(("$val"))

  #Read ADC 1.5v:LTC4286 1v:LTC4282
  if [ "$mbrev" -ge "$MB_DVT_BORAD_ID" ]
  then
    if [ "$data" -ge 4 ]
    then
      i2c_device_add 6 0x41 ltc4286
      kv set mb_hsc_source "$MB_3RD_SOURCE"
    else
      i2c_device_add 6 0x41 ltc4282
      kv set mb_hsc_source "$MB_2ND_SOURCE"
    fi
  else
    if [ "$data" -gt 4 ]
    then
      i2c_device_add 6 0x41 ltc4286
      kv set mb_hsc_source "$MB_3RD_SOURCE"
    else
      i2c_device_add 6 0x41 ltc4282
      kv set mb_hsc_source "$MB_2ND_SOURCE"
    fi
  fi

  #HSC Temp
  i2c_device_add 25 0x4C lm75
  #HSC module's eeprom
  i2c_device_add 6 0x51 24c64
  kv set mb_hsc_module "1"
}

#ADC
probe_adc_ti() {
  i2cset -f -y 20 0x1d 0x0b 0x02
  i2c_device_add 20 0x1d adc128d818
  i2cset -f -y 26 0x1d 0x0b 0x02
  i2c_device_add 26 0x1d adc128d818
  kv set mb_adc_source "$MB_1ST_SOURCE"

}

probe_adc_maxim() {
  i2c_device_add 20 0x35 max11617
  i2c_device_add 26 0x35 max11617
  kv set mb_adc_source "$MB_2ND_SOURCE"
}

#VR
probe_vr_raa() {
  i2c_device_add 20 0x61 isl69260
  i2c_device_add 20 0x62 isl69260
  i2c_device_add 20 0x63 isl69260
  i2c_device_add 20 0x72 isl69260
  i2c_device_add 20 0x74 isl69260
  i2c_device_add 20 0x75 isl69260
  kv set mb_vr_source "$MB_1ST_SOURCE"
}

probe_vr_xdpe() {
  i2c_device_add 20 0x4a xdpe152c4
  i2c_device_add 20 0x49 xdpe152c4
  i2c_device_add 20 0x4c xdpe152c4
  i2c_device_add 20 0x4d xdpe152c4
  i2c_device_add 20 0x4e xdpe152c4
  i2c_device_add 20 0x4f xdpe152c4
  kv set mb_vr_source "$MB_2ND_SOURCE"
}

probe_vr_mp2856() {
  #To do - Probe mp2856 driver
  i2c_device_add 20 0x49 mp2856
  i2c_device_add 20 0x4a mp2856
  i2c_device_add 20 0x4c mp2856
  i2c_device_add 20 0x4d mp2856
  i2c_device_add 20 0x4e mp2856
  i2c_device_add 20 0x4f mp2856
  kv set mb_vr_source "$MB_3RD_SOURCE"
}

#MB retimer VR
probe_mb_retimer_vr_isl() {
  i2c_device_add 20 0x60 isl69260
  i2c_device_add 20 0x76 isl69260
}

probe_mb_retimer_vr_xdpe() {
  i2c_device_add 20 0x60 xdpe152c4
  i2c_device_add 20 0x76 xdpe152c4
}

#MB DPM
# rev_id[2:0] | ID2 | ID1 | ID0 |
# 000         |  0  |  0  |  0  | EVT  (no retimer)
# 001         |  0  |  0  |  1  | DVT1 (8 retimer)
# 010         |  0  |  1  |  0  | DVT2 (2 retimer)
# 011         |  0  |  1  |  1  | PVT  (TBD)

mbrev=$(kv get mb_rev)
MB_DVT_BOARD_ID="1"
MB_PVT_BOARD_ID="3"

if [ "$mbrev" -ge "$MB_DVT_BOARD_ID" ]; then
  if [ "$mb_sku" -ne "$config8" ] &&
     [ "$mb_sku" -ne "$config9" ] &&
     [ "$mb_sku" -ne "$config12" ]; then
    i2c_device_add 34 0x41 ina230
    i2c_device_add 34 0x42 ina230
    i2c_device_add 34 0x43 ina230
    i2c_device_add 34 0x44 ina230
    i2c_device_add 34 0x45 ina230
  fi
  kv set mb_dpm_source "$MB_1ST_SOURCE"
fi

if [ "$mbrev" -eq "$MB_DVT_BOARD_ID" ] &&
   [ "$mbrev" -ge "$MB_PVT_BOARD_ID" ]; then
  probe_mb_retimer_vr
fi

if [ "$mb_sku" -eq "$config0" ]; then
  probe_hsc_mp5990
  probe_adc_ti
  probe_vr_raa
  probe_mb_retimer_vr_isl
elif [ "$mb_sku" -eq "$config1" ]; then
  probe_hsc_ltc
  probe_adc_maxim
  probe_vr_xdpe
  probe_mb_retimer_vr_xdpe
elif [ "$mb_sku" -eq "$config4" ]; then
  probe_hsc_mp5990
  probe_adc_ti
  probe_vr_mp2856
  probe_mb_retimer_vr_isl
# Artemis config
elif [ "$mb_sku" -eq "$config8" ]; then
  probe_hsc_ltc
  probe_adc_ti
  probe_vr_raa
  probe_mb_retimer_vr_isl
elif [ "$mb_sku" -eq "$config9" ]; then
  probe_hsc_ltc
  probe_adc_ti
  probe_vr_xdpe
  probe_mb_retimer_vr_xdpe
elif [ "$mb_sku" -eq "$config11" ]; then
  probe_hsc_ltc
  probe_adc_maxim
  probe_vr_raa
  probe_mb_retimer_vr_isl
elif [ "$mb_sku" -eq "$config12" ]; then
  probe_hsc_mp5990
  probe_adc_ti
  probe_vr_xdpe
  probe_mb_retimer_vr_xdpe
fi

if ! i2cget -f -y 0 0x70 0 >/dev/null 2>&1; then
  kv set apml_mux 0
else
  i2cset -f -y 0 0x70 0x46 0x01
  i2cset -f -y 0 0x70 0x40 0xc0
  i2cset -f -y 0 0x70 0x41 0xc0
  kv set apml_mux 1
fi
