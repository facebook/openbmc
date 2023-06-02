#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
. /usr/local/fbpackages/utils/ast-functions

rebind_i2c_dev() {
  dev="$1-00$2"
  dri=$3

  if [ ! -L "${SYSFS_I2C_DEVICES}/$dev/driver" ]; then
    if i2c_bind_driver "$dri" "$dev" >/dev/null; then
      echo "rebind $dev to driver $dri successfully"
    fi
  fi
}
#echo set SGPIO ready
gpio_set FM_BMC_SGPIO_READY_N 1

echo "Probe MB MUX"
rebind_i2c_dev 1 70 pca954x
rebind_i2c_dev 5 70 pca954x
rebind_i2c_dev 8 72 pca954x

echo "Probe PDBH MUX"
rebind_i2c_dev 37 71 pca954x

#MB TEMP Device"
i2c_device_add 21 0x48 stlm75
i2c_device_add 22 0x48 stlm75
i2c_device_add 23 0x48 stlm75
i2c_device_add 24 0x48 stlm75

MB_1ST_SOURCE="0"

#MB Expender
i2c_device_add 29 0x74 pca9539
# I/O Expander PCA9539 0xE8 BIC and GPU
gpio_export_ioexp 29-0074 RST_USB_HUB             0
gpio_export_ioexp 29-0074 RST_SWB_BIC_N           1
gpio_export_ioexp 29-0074 SWB_CABLE_PRSNT_G_N     4
gpio_export_ioexp 29-0074 GPU_CABLE_PRSNT_H_N     5
gpio_export_ioexp 29-0074 SWB_CABLE_PRSNT_F_N     6
gpio_export_ioexp 29-0074 SWB_CABLE_PRSNT_C_N     7

gpio_export_ioexp 29-0074 GPU_CABLE_PRSNT_D_N     8
gpio_export_ioexp 29-0074 SWB_CABLE_PRSNT_B_N     9
gpio_export_ioexp 29-0074 GPU_BASE_ID0            10
gpio_export_ioexp 29-0074 GPU_BASE_ID1            11
gpio_export_ioexp 29-0074 GPU_PEX_STRAP0          12
gpio_export_ioexp 29-0074 GPU_PEX_STRAP1          13
gpio_export_ioexp 29-0074 GPU_CABLE_PRSNT_A_N     14
gpio_export_ioexp 29-0074 GPU_CABLE_PRSNT_E_N     15

#MB FRU
i2c_device_add 33 0x51 24c64

# Swith Board
echo "Probe SWB Device"
SWB_1ST_SOURCE="0"
SWB_2ND_SOURCE="1"
SWB_3RD_SOURCE="2"
SWB_4TH_SOURCE="3"

# BIC I/O Expander PCA9539 0xEC BIC
i2c_device_add 32 0x76 pca9539

gpio_export_ioexp 32-0076 BIC_FWSPICK        0
gpio_export_ioexp 32-0076 BIC_UART_BMC_SEL   1

#Set IO Expender
gpio_set BIC_FWSPICK 0
gpio_set RST_SWB_BIC_N 1
gpio_set BIC_UART_BMC_SEL 0


bic_ready=$(gpio_get FM_SWB_BIC_READY_ISO_R_N)
#SWB HSC
if [ "$bic_ready" -eq 0 ]; then
  cnt=3
  while [ $cnt -ne 0 ]
  do
    sleep 1
    str=$(pldmd-util --bus 3 -e 0x0a raw 0x02 0x11 0xf0 0x00 0x01 |grep "PLDM Data")
    rev=$?
    if [ "$rev" -eq 1 ]; then
      cnt=$(("$cnt"-1))
      val=0
    else
      IFS=' ' read -ra  array <<< "$str"
      cnt=0
      for i in "${array[@]}"
      do
        array["$cnt"]=${i:2}
        cnt=$(("$cnt"+1))
      done

      int=$(((16#${array[9]}|16#${array[10]} << 8)*1000))
      dec=$((16#${array[11]}|16#${array[12]} << 8))
      val=$(("$int"+"$dec"))
      break;
    fi
  done

#kv set swb_hsc_module "0"
  if [ "$val" -gt 750 ] && [ "$val" -lt 1250 ]
  then
    kv set swb_hsc_source "$SWB_2ND_SOURCE" #ltc4282
    kv set swb_hsc_module "1"
  elif [ "$val" -gt 1250 ]
  then
    kv set swb_hsc_source "$SWB_3RD_SOURCE" #ltc4287
    kv set swb_hsc_module "1"
  else
    kv set swb_hsc_source "$SWB_1ST_SOURCE" #mp5990
  fi

#SWB VR
  cnt=3
  while [ "$cnt" -ne 0 ]
  do
    sleep 1
    str=$(pldmd-util --bus 3 -e 0x0a raw 0x02 0x11 0xf1 0x00 0x01 |grep "PLDM Data")
    rev=$?
    if [ "$rev" -eq 1 ]; then
      cnt=$(("$cnt"-1))
    else
      IFS=' ' read -ra  array <<< "$str"
      cnt=0
      for i in "${array[@]}"
      do
        array["$cnt"]=${i:2}
        cnt=$(("$cnt"+1))
      done

      int=$(((16#${array[9]}|16#${array[10]} << 8)*1000))
      dec=$((16#${array[11]}|16#${array[12]} << 8))
      val=$(("$int"+"$dec"))
      break;
    fi
  done

  if [ "$val" -gt 250 ] && [ "$val" -lt 750 ]
  then
    kv set swb_vr_source "$SWB_2ND_SOURCE" #INF
  elif [ "$val" -gt 750 ] && [ "$val" -lt 1250 ]
  then
    kv set swb_vr_source "$SWB_3RD_SOURCE" #MPS
  elif [ "$val" -gt 1250 ] && [ "$val" -lt 1750 ]
  then
    kv set swb_vr_source "$SWB_4TH_SOURCE" #TI
  else
    kv set swb_vr_source "$SWB_1ST_SOURCE" #RAA
  fi
fi

VPDB_EVT2="2"
VPDB_PVT="5"
VPDB_DISCRETE_PVT="3"
VPDB_1ST_SOURCE="0"
VPDB_2ND_SOURCE="1"
VPDB_3RD_SOURCE="2"
VPDB_HSC_MAIN="0"
#VPDB_HSC_SECOND="1"

# VPDB
echo "Probe VPDB Device"
#VPDB Expender
i2c_device_add 36 0x22 pca9555
gpio_export_ioexp 36-0022  HPDB_PRESENT     9
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_0  10
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_1  11
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_2  12
gpio_export_ioexp 36-0022  VPDB_SKU_ID_0    13
gpio_export_ioexp 36-0022  VPDB_SKU_ID_1    14
gpio_export_ioexp 36-0022  VPDB_SKU_ID_2    15

i2cget -f -y 36 0x25 0x00
rev=$?
if [ "$rev" -eq 0 ]; then
  gpio_export_ioexp 36-0025  VPDB_BOARD_ID_3_IO   0
  gpio_export_ioexp 36-0025  VPDB_SKU_ID_3_IO     15
  VPDB_BOARD_ID_3=$VPDB_BOARD_ID_3_IO
  VPDB_SKU_ID_3=$VPDB_SKU_ID_3_IO
else
  VPDB_BOARD_ID_3=0
  VPDB_SKU_ID_3=0
fi

kv set vpdb_rev "$((VPDB_BOARD_ID_3 << 3 |
                    $(gpio_get VPDB_BOARD_ID_2) << 2 |
                    $(gpio_get VPDB_BOARD_ID_1) << 1 |
                    $(gpio_get VPDB_BOARD_ID_0)))"

kv set vpdb_sku "$((VPDB_SKU_ID_3 << 3 |
                    $(gpio_get VPDB_SKU_ID_2) << 2 |
                    $(gpio_get VPDB_SKU_ID_1) << 1 |
                    $(gpio_get VPDB_SKU_ID_0)))"

#VPDB ADM1272/LTC4286 CONFIG
vpdb_hsc=$(gpio_get VPDB_SKU_ID_2)
vrev=$(kv get vpdb_rev)


if [ "$vpdb_hsc" -eq "$VPDB_HSC_MAIN" ] && [ "$vrev" -gt 1 ]; then
  i2c_device_add 38 0x44 ltc4286
  kv set vpdb_hsc_source "$VPDB_1ST_SOURCE"
else
  i2cset -y -f 38 0x10 0xd4 0x3F1F w
  i2c_device_add 38 0x10 adm1272
  kv set vpdb_hsc_source "$VPDB_2ND_SOURCE"
fi

vsku=$(kv get vpdb_sku)

if [ "$vsku" -eq "2" ] || [ "$vsku" -eq "5" ]; then
  brick_driver="raa228006"
  i2c_device_add 38 0x54 $brick_driver
  kv set vpdb_brick_source "$VPDB_3RD_SOURCE"

  if [ "$vrev" -gt "$VPDB_DISCRETE_PVT" ]; then
    if [ "$(gpio_get VPDB_SKU_ID_0)" -eq $VPDB_1ST_SOURCE ]; then
      i2c_device_add 36 0x67 ltc2945
      i2c_device_add 36 0x68 ltc2945
      kv set vpdb_adc_source "$VPDB_1ST_SOURCE"
    else
      i2c_device_add 36 0x40 ina238
      i2c_device_add 36 0x41 ina238
      kv set vpdb_adc_source "$VPDB_2ST_SOURCE"
    fi
  fi
else
  brick_driver="bmr491"
  vpdb_brick=$(gpio_get VPDB_SKU_ID_1)

  if [ "$vpdb_brick" -eq "$VPDB_1ST_SOURCE" ]; then
    brick_driver="pmbus"
    kv set vpdb_brick_source "$VPDB_1ST_SOURCE"
  else
    kv set vpdb_brick_source "$VPDB_2ND_SOURCE"
  fi

  vpdb_rev=$(kv get vpdb_rev)
  if [ "$vpdb_rev" -ge $VPDB_EVT2 ]; then
    i2c_device_add 38 0x67 $brick_driver
    i2c_device_add 38 0x68 $brick_driver
    i2c_device_add 38 0x69 $brick_driver
  else
    i2c_device_add 38 0x69 $brick_driver
    i2c_device_add 38 0x6a $brick_driver
    i2c_device_add 38 0x6b $brick_driver
  fi

  #vpdb ADC sensors
  if [ "$vrev" -gt "$VPDB_PVT" ]; then
    if [ "$(gpio_get VPDB_SKU_ID_0)" -eq $VPDB_1ST_SOURCE ]; then
      i2c_device_add 36 0x67 ltc2945
      i2c_device_add 36 0x68 ltc2945
      kv set vpdb_adc_source "$VPDB_1ST_SOURCE"
    else
      i2c_device_add 36 0x40 ina238
      i2c_device_add 36 0x41 ina238
      kv set vpdb_adc_source "$VPDB_2ST_SOURCE"
    fi
  fi
fi
# VPDB FRU
i2c_device_add 36 0x52 24c64 #VPDB FRU



#***HPDB Board Device Probe***
HPDB_PVT=5
HPDB_1ST_SOURCE="0"
HPDB_2ND_SOURCE="1"
#HPDB_3RD_SOURCE="2"
HPDB_HSC_MAIN="0"
#HPDB_HSC_SECOND="1"

echo "Probe HPDB Device"
#HPDB ID Expender
i2c_device_add 37 0x23 pca9555
gpio_export_ioexp 37-0023  FAN_BP1_PRSNT_N  2
gpio_export_ioexp 37-0023  FAN_BP2_PRSNT_N  3
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_0  10
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_1  11
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_2  12
gpio_export_ioexp 37-0023  HPDB_SKU_ID_0    13
gpio_export_ioexp 37-0023  HPDB_SKU_ID_1    14
gpio_export_ioexp 37-0023  HPDB_SKU_ID_2    15

i2c_device_add 37 0x25 pca9555
gpio_export_ioexp 37-0025  FM_HS1_EN_BUSBAR_BUF  1
gpio_export_ioexp 37-0025  FM_HS2_EN_BUSBAR_BUF  3

i2cget -f -y 37 0x25 0x00
rev=$?
if [ "$rev" -eq 0 ]; then
  gpio_export_ioexp 37-0025  HPDB_BOARD_ID_3_IO    15
  HPDB_BOARD_ID_3=$HPDB_BOARD_ID_3_IO
else
  HPDB_BOARD_ID_3=0
fi


hpdb_rc="$(($(gpio_get HPDB_BOARD_ID_2) << 2 |
      $(gpio_get HPDB_BOARD_ID_1) << 1 |
      $(gpio_get HPDB_BOARD_ID_0)))"


if [ $hpdb_rc -ge 11 ] && [ $hpdb_rc -le 15 ]; then
   HPDB_BOARD_ID_3=0
fi

kv set hpdb_rev "$((HPDB_BOARD_ID_3 << 3 | hpdb_rc))"



kv set hpdb_sku "$(($(gpio_get HPDB_SKU_ID_2) << 2 |
                    $(gpio_get HPDB_SKU_ID_1) << 1 |
                    $(gpio_get HPDB_SKU_ID_0)))"

# HPDB ADM1272/LTC4286 CONFIG
hpdb_hsc=$(gpio_get HPDB_SKU_ID_2)
hrev=$(kv get hpdb_rev)

if [ "$hpdb_hsc" -eq "$HPDB_HSC_MAIN" ] && [ "$hrev" -gt 1 ]; then
  i2c_device_add 39 0x40 ltc4286
  i2c_device_add 39 0x41 ltc4286
  kv set hpdb_hsc_source "$HPDB_1ST_SOURCE"
else
  i2cset -y -f 39 0x13 0xd4 0x3F1F w
  i2cset -y -f 39 0x1c 0xd4 0x3F1F w
  i2c_device_add 39 0x13 adm1272
  i2c_device_add 39 0x1c adm1272
  kv set hpdb_hsc_source "$HPDB_2ND_SOURCE"
fi

if [ "$hrev" -gt "$HPDB_PVT" ]; then
  if [ "$(gpio_get HPDB_SKU_ID_1)" -eq "$HPDB_1ST_SOURCE" ]; then
    i2c_device_add 37 0x69 ltc2945
    i2c_device_add 37 0x6a ltc2945
    i2c_device_add 37 0x6b ltc2945
    i2c_device_add 37 0x6c ltc2945
    kv set hpdb_adc_source "$HPDB_1ST_SOURCE"
  else
    i2c_device_add 37 0x42 ina238
    i2c_device_add 37 0x43 ina238
    i2c_device_add 37 0x44 ina238
    i2c_device_add 37 0x45 ina238
    kv set hpdb_adc_source "$HPDB_2ND_SOURCE"
  fi
fi

# HPDB FRU
i2c_device_add 37 0x51 24c64



#***FAN Board Device Probe***
BP_1ST_SOURCE="0"
BP_2ND_SOURCE="1"
BP_FAN_MAIN="0"
echo "Probe FAN Board Device"
# FAN_BP1 I/O Expander PCA9555
i2c_device_add 40 0x21 pca9555
gpio_export_ioexp 40-0021 FAN_BP1_SKU_ID_0    8
gpio_export_ioexp 40-0021 FAN_BP1_SKU_ID_1    9
gpio_export_ioexp 40-0021 FAN_BP1_SKU_ID_2    10

kv set fan_bp1_fan_sku "$(($(gpio_get FAN_BP1_SKU_ID_0)))"

# FAN_BP2 I/O Expander PCA9555
i2c_device_add 41 0x21 pca9555
gpio_export_ioexp 41-0021 FAN_BP2_SKU_ID_0    8
gpio_export_ioexp 41-0021 FAN_BP2_SKU_ID_1    9
gpio_export_ioexp 41-0021 FAN_BP2_SKU_ID_2    10

kv set fan_bp2_fan_sku "$(($(gpio_get FAN_BP2_SKU_ID_0)))"

fan_bp1_fan=$(kv get fan_bp1_fan_sku)
if [ "$fan_bp1_fan" -eq "$BP_FAN_MAIN" ]; then
# Max31790
  # FAN_BP1 Max31790 FAN CHIP
  i2cset -f -y 40 0x20 0x01 0xbb
  i2cset -f -y 40 0x20 0x02 0x08
  i2cset -f -y 40 0x20 0x03 0x19
  i2cset -f -y 40 0x20 0x04 0x08
  i2cset -f -y 40 0x20 0x05 0x08
  i2cset -f -y 40 0x20 0x06 0x19
  i2cset -f -y 40 0x20 0x07 0x08
  i2cset -f -y 40 0x20 0x08 0x6C
  i2cset -f -y 40 0x20 0x09 0x6C
  i2cset -f -y 40 0x20 0x0A 0x6C
  i2cset -f -y 40 0x20 0x0B 0x6C
  i2cset -f -y 40 0x20 0x0C 0x6C
  i2cset -f -y 40 0x20 0x0D 0x6C

  i2cset -f -y 40 0x2f 0x01 0xbb
  i2cset -f -y 40 0x2f 0x02 0x08
  i2cset -f -y 40 0x2f 0x03 0x19
  i2cset -f -y 40 0x2f 0x04 0x08
  i2cset -f -y 40 0x2f 0x05 0x08
  i2cset -f -y 40 0x2f 0x06 0x19
  i2cset -f -y 40 0x2f 0x07 0x08
  i2cset -f -y 40 0x2f 0x08 0x6C
  i2cset -f -y 40 0x2f 0x09 0x6C
  i2cset -f -y 40 0x2f 0x0A 0x6C
  i2cset -f -y 40 0x2f 0x0B 0x6C
  i2cset -f -y 40 0x2f 0x0C 0x6C
  i2cset -f -y 40 0x2f 0x0D 0x6C

  i2c_device_add 40 0x20 max31790
  i2c_device_add 40 0x2f max31790
  kv set fan_bp1_fan_chip_source "$BP_1ST_SOURCE"
else
  # FAN_BP1 NCT3763Y FAN CHIP
  # Config FAN PWM and FAIN FAN_BP1
  i2cset -f -y -a 40 0x01 0x20 0xA9
  i2cset -f -y -a 40 0x01 0x21 0x99
  i2cset -f -y -a 40 0x01 0x22 0x9A
  i2cset -f -y -a 40 0x01 0x38 0x51
  i2cset -f -y -a 40 0x01 0x39 0x04
  i2cset -f -y -a 40 0x01 0x41 0x0B
  i2cset -f -y -a 40 0x01 0x42 0xAE

  i2cset -f -y -a 40 0x02 0x20 0xA9
  i2cset -f -y -a 40 0x02 0x21 0x99
  i2cset -f -y -a 40 0x02 0x22 0x9A
  i2cset -f -y -a 40 0x02 0x38 0x51
  i2cset -f -y -a 40 0x02 0x39 0x04
  i2cset -f -y -a 40 0x02 0x41 0x0B
  i2cset -f -y -a 40 0x02 0x42 0xAE

  # Set FAN 25KHZ
  i2cset -f -y -a 40 0x01 0xB0 0x00
  i2cset -f -y -a 40 0x01 0xB1 0x00
  i2cset -f -y -a 40 0x01 0xB2 0x00
  i2cset -f -y -a 40 0x01 0xB3 0x00
  i2cset -f -y -a 40 0x01 0xB4 0x00
  i2cset -f -y -a 40 0x01 0xB5 0x00
  i2cset -f -y -a 40 0x01 0xB6 0x00
  i2cset -f -y -a 40 0x01 0xB7 0x00

  i2cset -f -y -a 40 0x01 0x91 0x04
  i2cset -f -y -a 40 0x01 0x93 0x04
  i2cset -f -y -a 40 0x01 0x97 0x04
  i2cset -f -y -a 40 0x01 0xA3 0x04
  i2cset -f -y -a 40 0x01 0xA5 0x04
  i2cset -f -y -a 40 0x01 0xA7 0x04
  i2cset -f -y -a 40 0x01 0xAB 0x04
  i2cset -f -y -a 40 0x01 0xAF 0x04

  i2cset -f -y -a 40 0x02 0xB0 0x00
  i2cset -f -y -a 40 0x02 0xB1 0x00
  i2cset -f -y -a 40 0x02 0xB2 0x00
  i2cset -f -y -a 40 0x02 0xB3 0x00
  i2cset -f -y -a 40 0x02 0xB4 0x00
  i2cset -f -y -a 40 0x02 0xB5 0x00
  i2cset -f -y -a 40 0x02 0xB6 0x00
  i2cset -f -y -a 40 0x02 0xB7 0x00

  i2cset -f -y -a 40 0x02 0x91 0x04
  i2cset -f -y -a 40 0x02 0x93 0x04
  i2cset -f -y -a 40 0x02 0x97 0x04
  i2cset -f -y -a 40 0x02 0xA3 0x04
  i2cset -f -y -a 40 0x02 0xA5 0x04
  i2cset -f -y -a 40 0x02 0xA7 0x04
  i2cset -f -y -a 40 0x02 0xAB 0x04
  i2cset -f -y -a 40 0x02 0xAF 0x04
  kv set fan_bp1_fan_chip_source "$BP_2ND_SOURCE"
fi

fan_bp2_fan=$(kv get fan_bp2_fan_sku)
if [ "$fan_bp2_fan" -eq "$BP_1ST_SOURCE" ]; then
  # FAN_BP2 MAX31790 FAN CHIP
  i2cset -f -y 41 0x20 0x01 0xbb
  i2cset -f -y 41 0x20 0x02 0x08
  i2cset -f -y 41 0x20 0x03 0x19
  i2cset -f -y 41 0x20 0x04 0x08
  i2cset -f -y 41 0x20 0x05 0x08
  i2cset -f -y 41 0x20 0x06 0x19
  i2cset -f -y 41 0x20 0x07 0x08
  i2cset -f -y 41 0x20 0x08 0x6C
  i2cset -f -y 41 0x20 0x09 0x6C
  i2cset -f -y 41 0x20 0x0A 0x6C
  i2cset -f -y 41 0x20 0x0B 0x6C
  i2cset -f -y 41 0x20 0x0C 0x6C
  i2cset -f -y 41 0x20 0x0D 0x6C

  i2cset -f -y 41 0x2f 0x01 0xbb
  i2cset -f -y 41 0x2f 0x02 0x08
  i2cset -f -y 41 0x2f 0x03 0x19
  i2cset -f -y 41 0x2f 0x04 0x08
  i2cset -f -y 41 0x2f 0x05 0x08
  i2cset -f -y 41 0x2f 0x06 0x19
  i2cset -f -y 41 0x2f 0x07 0x08
  i2cset -f -y 41 0x2f 0x08 0x6C
  i2cset -f -y 41 0x2f 0x09 0x6C
  i2cset -f -y 41 0x2f 0x0A 0x6C
  i2cset -f -y 41 0x2f 0x0B 0x6C
  i2cset -f -y 41 0x2f 0x0C 0x6C
  i2cset -f -y 41 0x2f 0x0D 0x6C

  i2c_device_add 41 0x20 max31790
  i2c_device_add 41 0x2f max31790
  kv set fan_bp2_fan_chip_source "$BP_1ST_SOURCE"
else
  # FAN_BP2 NCT3763Y FAN CHIP
  # Config FAN PWM and FAIN FAN_BP2
  i2cset -f -y -a 41 0x01 0x20 0xA9
  i2cset -f -y -a 41 0x01 0x21 0x99
  i2cset -f -y -a 41 0x01 0x22 0x9A
  i2cset -f -y -a 41 0x01 0x38 0x51
  i2cset -f -y -a 41 0x01 0x39 0x04
  i2cset -f -y -a 41 0x01 0x41 0x0B
  i2cset -f -y -a 41 0x01 0x42 0xAE

  i2cset -f -y -a 41 0x02 0x20 0xA9
  i2cset -f -y -a 41 0x02 0x21 0x99
  i2cset -f -y -a 41 0x02 0x22 0x9A
  i2cset -f -y -a 41 0x02 0x38 0x51
  i2cset -f -y -a 41 0x02 0x39 0x04
  i2cset -f -y -a 41 0x02 0x41 0x0B
  i2cset -f -y -a 41 0x02 0x42 0xAE
  # Set FAN 25KHZ
  i2cset -f -y -a 41 0x01 0xB0 0x00
  i2cset -f -y -a 41 0x01 0xB1 0x00
  i2cset -f -y -a 41 0x01 0xB2 0x00
  i2cset -f -y -a 41 0x01 0xB3 0x00
  i2cset -f -y -a 41 0x01 0xB4 0x00
  i2cset -f -y -a 41 0x01 0xB5 0x00
  i2cset -f -y -a 41 0x01 0xB6 0x00
  i2cset -f -y -a 41 0x01 0xB7 0x00

  i2cset -f -y -a 41 0x01 0x91 0x04
  i2cset -f -y -a 41 0x01 0x93 0x04
  i2cset -f -y -a 41 0x01 0x97 0x04
  i2cset -f -y -a 41 0x01 0xA3 0x04
  i2cset -f -y -a 41 0x01 0xA5 0x04
  i2cset -f -y -a 41 0x01 0xA7 0x04
  i2cset -f -y -a 41 0x01 0xAB 0x04
  i2cset -f -y -a 41 0x01 0xAF 0x04

  i2cset -f -y -a 41 0x02 0xB0 0x00
  i2cset -f -y -a 41 0x02 0xB1 0x00
  i2cset -f -y -a 41 0x02 0xB2 0x00
  i2cset -f -y -a 41 0x02 0xB3 0x00
  i2cset -f -y -a 41 0x02 0xB4 0x00
  i2cset -f -y -a 41 0x02 0xB5 0x00
  i2cset -f -y -a 41 0x02 0xB6 0x00
  i2cset -f -y -a 41 0x02 0xB7 0x00

  i2cset -f -y -a 41 0x02 0x91 0x04
  i2cset -f -y -a 41 0x02 0x93 0x04
  i2cset -f -y -a 41 0x02 0x97 0x04
  i2cset -f -y -a 41 0x02 0xA3 0x04
  i2cset -f -y -a 41 0x02 0xA5 0x04
  i2cset -f -y -a 41 0x02 0xA7 0x04
  i2cset -f -y -a 41 0x02 0xAB 0x04
  i2cset -f -y -a 41 0x02 0xAF 0x04
  kv set fan_bp2_fan_chip_source "$BP_2ND_SOURCE"
fi


# FAN LED Device"
rebind_i2c_dev 40 62 leds-pca955x
rebind_i2c_dev 41 62 leds-pca955x

# FAN Board FRU
i2c_device_add 40 0x56 24c64 #FAN_BP1 FRU
i2c_device_add 41 0x56 24c64 #FAN_BP2 FRU

gpio_export_ioexp 40-0021 FAN0_PRESENT   7
gpio_export_ioexp 40-0021 FAN4_PRESENT   6
gpio_export_ioexp 40-0021 FAN8_PRESENT   5
gpio_export_ioexp 40-0021 FAN12_PRESENT  4
gpio_export_ioexp 40-0021 FAN1_PRESENT   0
gpio_export_ioexp 40-0021 FAN5_PRESENT   1
gpio_export_ioexp 40-0021 FAN9_PRESENT   2
gpio_export_ioexp 40-0021 FAN13_PRESENT  3

gpio_export_ioexp 41-0021 FAN2_PRESENT   7
gpio_export_ioexp 41-0021 FAN6_PRESENT   6
gpio_export_ioexp 41-0021 FAN10_PRESENT  5
gpio_export_ioexp 41-0021 FAN14_PRESENT  4
gpio_export_ioexp 41-0021 FAN3_PRESENT   0
gpio_export_ioexp 41-0021 FAN7_PRESENT   1
gpio_export_ioexp 41-0021 FAN11_PRESENT  2
gpio_export_ioexp 41-0021 FAN15_PRESENT  3

# I/O Expander PCA955X FAN LED
gpio_export_ioexp 40-0062  FAN1_LED_GOOD   0
gpio_export_ioexp 40-0062  FAN1_LED_FAIL   1
gpio_export_ioexp 40-0062  FAN5_LED_GOOD   2
gpio_export_ioexp 40-0062  FAN5_LED_FAIL   3
gpio_export_ioexp 40-0062  FAN9_LED_GOOD   4
gpio_export_ioexp 40-0062  FAN9_LED_FAIL   5
gpio_export_ioexp 40-0062  FAN13_LED_GOOD  6
gpio_export_ioexp 40-0062  FAN13_LED_FAIL  7
gpio_export_ioexp 40-0062  FAN12_LED_GOOD  8
gpio_export_ioexp 40-0062  FAN12_LED_FAIL  9
gpio_export_ioexp 40-0062  FAN8_LED_GOOD   10
gpio_export_ioexp 40-0062  FAN8_LED_FAIL   11
gpio_export_ioexp 40-0062  FAN4_LED_GOOD   12
gpio_export_ioexp 40-0062  FAN4_LED_FAIL   13
gpio_export_ioexp 40-0062  FAN0_LED_GOOD   14
gpio_export_ioexp 40-0062  FAN0_LED_FAIL   15

gpio_export_ioexp 41-0062  FAN3_LED_GOOD   0
gpio_export_ioexp 41-0062  FAN3_LED_FAIL   1
gpio_export_ioexp 41-0062  FAN7_LED_GOOD   2
gpio_export_ioexp 41-0062  FAN7_LED_FAIL   3
gpio_export_ioexp 41-0062  FAN11_LED_GOOD  4
gpio_export_ioexp 41-0062  FAN11_LED_FAIL  5
gpio_export_ioexp 41-0062  FAN15_LED_GOOD  6
gpio_export_ioexp 41-0062  FAN15_LED_FAIL  7
gpio_export_ioexp 41-0062  FAN14_LED_GOOD  8
gpio_export_ioexp 41-0062  FAN14_LED_FAIL  9
gpio_export_ioexp 41-0062  FAN10_LED_GOOD  10
gpio_export_ioexp 41-0062  FAN10_LED_FAIL  11
gpio_export_ioexp 41-0062  FAN6_LED_GOOD   12
gpio_export_ioexp 41-0062  FAN6_LED_FAIL   13
gpio_export_ioexp 41-0062  FAN2_LED_GOOD   14
gpio_export_ioexp 41-0062  FAN2_LED_FAIL   15

gpio_set  FAN0_LED_GOOD  1
gpio_set  FAN1_LED_GOOD  1
gpio_set  FAN2_LED_GOOD  1
gpio_set  FAN3_LED_GOOD  1
gpio_set  FAN4_LED_GOOD  1
gpio_set  FAN5_LED_GOOD  1
gpio_set  FAN6_LED_GOOD  1
gpio_set  FAN7_LED_GOOD  1
gpio_set  FAN8_LED_GOOD  1
gpio_set  FAN9_LED_GOOD  1
gpio_set  FAN10_LED_GOOD 1
gpio_set  FAN11_LED_GOOD 1
gpio_set  FAN12_LED_GOOD 1
gpio_set  FAN13_LED_GOOD 1
gpio_set  FAN14_LED_GOOD 1
gpio_set  FAN15_LED_GOOD 1

gpio_set  FAN0_LED_FAIL  0
gpio_set  FAN1_LED_FAIL  0
gpio_set  FAN2_LED_FAIL  0
gpio_set  FAN3_LED_FAIL  0
gpio_set  FAN4_LED_FAIL  0
gpio_set  FAN5_LED_FAIL  0
gpio_set  FAN6_LED_FAIL  0
gpio_set  FAN7_LED_FAIL  0
gpio_set  FAN8_LED_FAIL  0
gpio_set  FAN9_LED_FAIL  0
gpio_set  FAN10_LED_FAIL 0
gpio_set  FAN11_LED_FAIL 0
gpio_set  FAN12_LED_FAIL 0
gpio_set  FAN13_LED_FAIL 0
gpio_set  FAN14_LED_FAIL 0
gpio_set  FAN15_LED_FAIL 0

/usr/local/bin/fan-util --set 70

#GPU Reset USB Hub
if [ "$(is_bmc_por)" -eq 1 ]; then
  # set gpio 98 LOW
  pldmd-util -b 3 -e 0x0a raw 0x02 0x39 0x62 0xFF 0x02 0x00 0x00 0x01 0x01
  # add delay
  sleep 0.5
  # set gpio 98 HIGH
  pldmd-util -b 3 -e 0x0a raw 0x02 0x39 0x62 0xFF 0x02 0x00 0x00 0x01 0x02
  sleep 3
fi

