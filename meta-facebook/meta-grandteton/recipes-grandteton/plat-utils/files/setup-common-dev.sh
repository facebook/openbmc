#!/bin/sh
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

#MB ADM128D
i2cset -f -y 20 0x1d 0x0b 0x02
i2c_device_add 20 0x1d adc128d818

#MB Expender
i2c_device_add 29 0x74 pca9539

#MB FRU
i2c_device_add 33 0x51 24c64


# Swith Board
echo "Probe SWB Device"
# BIC I/O Expander PCA9539 0xEC BIC
i2c_device_add 32 0x76 pca9539

gpio_export_ioexp 32-0076 BIC_FWSPICK        0
gpio_export_ioexp 32-0076 BIC_UART_BMC_SEL   1

# I/O Expander PCA9539 0xE8 BIC and GPU
gpio_export_ioexp 29-0074 RST_USB_HUB     0
gpio_export_ioexp 29-0074 RST_SWB_BIC_N   1
gpio_export_ioexp 29-0074 GPU_SMB1_ALERT  8
gpio_export_ioexp 29-0074 GPU_SMB2_ALERT  9
gpio_export_ioexp 29-0074 GPU_BASE_ID0    10
gpio_export_ioexp 29-0074 GPU_BASE_ID1    11
gpio_export_ioexp 29-0074 GPU_PEX_STRAP0  12
gpio_export_ioexp 29-0074 GPU_PEX_STRAP1  13

#Set IO Expender
gpio_set BIC_FWSPICK 0
gpio_set RST_SWB_BIC_N 1

VPDB_EVT2_BORAD_ID="2"
VPDB_MAIN_SOURCE="0"
HPDB_MAIN_SOURCE="0"

# VPDB
echo "Probe VPDB Device"
#VPDB Expender
i2c_device_add 36 0x22 pca9555
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_0  10
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_1  11
gpio_export_ioexp 36-0022  VPDB_BOARD_ID_2  12
gpio_export_ioexp 36-0022  VPDB_SKU_ID_0    13
gpio_export_ioexp 36-0022  VPDB_SKU_ID_1    14
gpio_export_ioexp 36-0022  VPDB_SKU_ID_2    15

kv set vpdb_rev "$(($(gpio_get VPDB_BOARD_ID_2) << 2 |
                   $(gpio_get VPDB_BOARD_ID_1) << 1 |
                   $(gpio_get VPDB_BOARD_ID_0)))"

kv set vpdb_sku "$(($(gpio_get VPDB_SKU_ID_2) << 2 |
                    $(gpio_get VPDB_SKU_ID_1) << 1 |
                    $(gpio_get VPDB_SKU_ID_0)))"

#VPDB ADM1272 CONFIG
i2cset -y -f 38 0x10 0xd4 0x3F1C w
i2c_device_add 38 0x10 adm1272

if [ "$(kv get vpdb_rev)" -eq $VPDB_EVT2_BORAD_ID ]; then
  i2c_device_add 38 0x67 pmbus
  i2c_device_add 38 0x68 pmbus
  i2c_device_add 38 0x69 pmbus
else
  i2c_device_add 38 0x69 pmbus
  i2c_device_add 38 0x6a pmbus
  i2c_device_add 38 0x6b pmbus
fi

if [ "$(gpio_get VPDB_SKU_ID_0)" -eq $VPDB_MAIN_SOURCE ]; then
  i2c_device_add 36 0x67 ltc2945
  i2c_device_add 36 0x68 ltc2945
else
  i2c_device_add 36 0x40 ina238
  i2c_device_add 36 0x41 ina238
fi

# VPDB FRU
i2c_device_add 36 0x52 24c64 #VPDB FRU


# HPDB
echo "Probe HPDB Device"
#HPDB ID Expender
i2c_device_add 37 0x23 pca9555
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_0  10
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_1  11
gpio_export_ioexp 37-0023  HPDB_BOARD_ID_2  12
gpio_export_ioexp 37-0023  HPDB_SKU_ID_0    13
gpio_export_ioexp 37-0023  HPDB_SKU_ID_1    14
gpio_export_ioexp 37-0023  HPDB_SKU_ID_2    15

kv set hpdb_rev "$(($(gpio_get HPDB_BOARD_ID_2) << 2 |
                    $(gpio_get HPDB_BOARD_ID_1) << 1 |
                    $(gpio_get HPDB_BOARD_ID_0)))"

kv set hpdb_sku "$(($(gpio_get HPDB_SKU_ID_2) << 2 |
                    $(gpio_get HPDB_SKU_ID_1) << 1 |
                    $(gpio_get HPDB_SKU_ID_0)))"

# HPDB ADM1272 CONFIG
i2cset -y -f 39 0x13 0xd4 0x3F1C w
i2cset -y -f 39 0x1c 0xd4 0x3F1C w
i2c_device_add 39 0x13 adm1272
i2c_device_add 39 0x1c adm1272

if [ "$(gpio_get HPDB_SKU_ID_0)" -eq $HPDB_MAIN_SOURCE ]; then
  i2c_device_add 37 0x69 ltc2945
  i2c_device_add 37 0x6a ltc2945
  i2c_device_add 37 0x6b ltc2945
  i2c_device_add 37 0x6c ltc2945
else
  i2c_device_add 37 0x42 ina238
  i2c_device_add 37 0x43 ina238
  i2c_device_add 37 0x44 ina238
  i2c_device_add 37 0x45 ina238
fi

# HPDB FRU
i2c_device_add 37 0x51 24c64


echo "Probe FAN Board Device"
# FAN Board
# BP0 I/O Expander PCA9555
i2c_device_add 40 0x21 pca9555
gpio_export_ioexp 40-0021 BP0_SKU_ID_0    8
gpio_export_ioexp 40-0021 BP0_SKU_ID_1    9
gpio_export_ioexp 40-0021 BP0_SKU_ID_2    10

kv set bp0_fan_sku "$(($(gpio_get BP0_SKU_ID_0)))"


# BP1 I/O Expander PCA9555
i2c_device_add 41 0x21 pca9555
gpio_export_ioexp 41-0021 BP1_SKU_ID_0    8
gpio_export_ioexp 41-0021 BP1_SKU_ID_1    9
gpio_export_ioexp 41-0021 BP1_SKU_ID_2    10

kv set bp1_fan_sku "$(($(gpio_get BP1_SKU_ID_0)))"


if [ "$(kv get bp0_fan_sku)" -eq "0" ]; then
# Max31790
  echo "Probe BP0 Max31790 FAN CHIP"
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
else
  echo "Probe BP0 NCT3763Y FAN CHIP"
  # Config FAN PWM and FAIN BP0
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
  i2cset -f -y -a 40 0x01 0x91 0x05
  i2cset -f -y -a 40 0x01 0x93 0x05
  i2cset -f -y -a 40 0x01 0x97 0x05
  i2cset -f -y -a 40 0x01 0xA3 0x05
  i2cset -f -y -a 40 0x01 0xA5 0x05
  i2cset -f -y -a 40 0x01 0xA7 0x05
  i2cset -f -y -a 40 0x01 0xAB 0x05
  i2cset -f -y -a 40 0x01 0xAF 0x05

  i2cset -f -y -a 40 0x02 0x91 0x05
  i2cset -f -y -a 40 0x02 0x93 0x05
  i2cset -f -y -a 40 0x02 0x97 0x05
  i2cset -f -y -a 40 0x02 0xA3 0x05
  i2cset -f -y -a 40 0x02 0xA5 0x05
  i2cset -f -y -a 40 0x02 0xA7 0x05
  i2cset -f -y -a 40 0x02 0xAB 0x05
  i2cset -f -y -a 40 0x02 0xAF 0x05
fi

if [ "$(kv get bp1_fan_sku)" -eq "0" ]; then
  echo "Probe BP1 MAX31790 FAN CHIP"
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
else
  echo "Probe BP1 NCT3763Y FAN CHIP"
  # Config FAN PWM and FAIN BP1
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
  i2cset -f -y -a 41 0x01 0x91 0x05
  i2cset -f -y -a 41 0x01 0x93 0x05
  i2cset -f -y -a 41 0x01 0x97 0x05
  i2cset -f -y -a 41 0x01 0xA3 0x05
  i2cset -f -y -a 41 0x01 0xA5 0x05
  i2cset -f -y -a 41 0x01 0xA7 0x05
  i2cset -f -y -a 41 0x01 0xAB 0x05
  i2cset -f -y -a 41 0x01 0xAF 0x05

  i2cset -f -y -a 41 0x02 0x91 0x05
  i2cset -f -y -a 41 0x02 0x93 0x05
  i2cset -f -y -a 41 0x02 0x97 0x05
  i2cset -f -y -a 41 0x02 0xA3 0x05
  i2cset -f -y -a 41 0x02 0xA5 0x05
  i2cset -f -y -a 41 0x02 0xA7 0x05
  i2cset -f -y -a 41 0x02 0xAB 0x05
  i2cset -f -y -a 41 0x02 0xAF 0x05
fi


echo "Probe FAN LED Device"
rebind_i2c_dev 40 62 leds-pca955x
rebind_i2c_dev 41 62 leds-pca955x

echo "Probe FRU Device"
# FAN Board FRU
i2c_device_add 40 0x56 24c64 #BP0 FRU
i2c_device_add 41 0x56 24c64 #BP1 FRU

gpio_export_ioexp 40-0021 FAN0_PRESENT   4
gpio_export_ioexp 40-0021 FAN4_PRESENT   5
gpio_export_ioexp 40-0021 FAN8_PRESENT   6
gpio_export_ioexp 40-0021 FAN12_PRESENT  7
gpio_export_ioexp 40-0021 FAN1_PRESENT   0
gpio_export_ioexp 40-0021 FAN5_PRESENT   1
gpio_export_ioexp 40-0021 FAN9_PRESENT   2
gpio_export_ioexp 40-0021 FAN13_PRESENT  3

gpio_export_ioexp 41-0021 FAN2_PRESENT   4
gpio_export_ioexp 41-0021 FAN6_PRESENT   5
gpio_export_ioexp 41-0021 FAN10_PRESENT  6
gpio_export_ioexp 41-0021 FAN14_PRESENT  7
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
