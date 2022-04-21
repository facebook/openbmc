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

echo "Probe MB MUX"
i2c_device_add 1 0x70  pca9548
i2c_device_add 5 0x70  pca9548

if /usr/sbin/i2cget -f -y 8 0x73;
then
  i2c_device_add 8 0x72  pca9543
  i2c_device_add 8 0x73  pca9543
else
  i2c_device_add 8 0x72  pca9545
fi

echo "Probe PDBH MUX"
i2c_device_add 37 0x71 pca9543

echo "Probe BP Mux"
i2c_device_add 40 0x70 pca9548
i2c_device_add 41 0x70 pca9548

echo "Probe TEMP Device"
i2c_device_add 1 0x4E stlm75
i2c_device_add 21 0x48 stlm75
i2c_device_add 22 0x48 stlm75
i2c_device_add 23 0x48 stlm75
i2c_device_add 24 0x48 stlm75

echo "Probe VR Device"
#CPU VR
i2c_device_add 20 0x60 isl69260
i2c_device_add 20 0x61 isl69260
i2c_device_add 20 0x63 isl69260
i2c_device_add 20 0x72 isl69260
i2c_device_add 20 0x74 isl69260
i2c_device_add 20 0x76 isl69260

#FAN Max 31790
i2cset -f -y 40 0x20 0x01 0xbb
i2cset -f -y 40 0x20 0x02 0x08
i2cset -f -y 40 0x20 0x03 0x19
i2cset -f -y 40 0x20 0x04 0x08
i2cset -f -y 40 0x20 0x05 0x08
i2cset -f -y 40 0x20 0x06 0x19
i2cset -f -y 40 0x20 0x07 0x08
i2cset -f -y 40 0x20 0x08 0x8C
i2cset -f -y 40 0x20 0x09 0x8C
i2cset -f -y 40 0x20 0x0A 0x8C
i2cset -f -y 40 0x20 0x0B 0x8C
i2cset -f -y 40 0x20 0x0C 0x8C
i2cset -f -y 40 0x20 0x0D 0x8C

i2cset -f -y 40 0x2f 0x01 0xbb
i2cset -f -y 40 0x2f 0x02 0x08
i2cset -f -y 40 0x2f 0x03 0x19
i2cset -f -y 40 0x2f 0x04 0x08
i2cset -f -y 40 0x2f 0x05 0x08
i2cset -f -y 40 0x2f 0x06 0x19
i2cset -f -y 40 0x2f 0x07 0x08
i2cset -f -y 40 0x2f 0x08 0x8C
i2cset -f -y 40 0x2f 0x09 0x8C
i2cset -f -y 40 0x2f 0x0A 0x8C
i2cset -f -y 40 0x2f 0x0B 0x8C
i2cset -f -y 40 0x2f 0x0C 0x8C
i2cset -f -y 40 0x2f 0x0D 0x8C

i2cset -f -y 41 0x20 0x01 0xbb
i2cset -f -y 41 0x20 0x02 0x08
i2cset -f -y 41 0x20 0x03 0x19
i2cset -f -y 41 0x20 0x04 0x08
i2cset -f -y 41 0x20 0x05 0x08
i2cset -f -y 41 0x20 0x06 0x19
i2cset -f -y 41 0x20 0x07 0x08
i2cset -f -y 41 0x20 0x08 0x8C
i2cset -f -y 41 0x20 0x09 0x8C
i2cset -f -y 41 0x20 0x0A 0x8C
i2cset -f -y 41 0x20 0x0B 0x8C
i2cset -f -y 41 0x20 0x0C 0x8C
i2cset -f -y 41 0x20 0x0D 0x8C

i2cset -f -y 41 0x2f 0x01 0xbb
i2cset -f -y 41 0x2f 0x02 0x08
i2cset -f -y 41 0x2f 0x03 0x19
i2cset -f -y 41 0x2f 0x04 0x08
i2cset -f -y 41 0x2f 0x05 0x08
i2cset -f -y 41 0x2f 0x06 0x19
i2cset -f -y 41 0x2f 0x07 0x08
i2cset -f -y 41 0x2f 0x08 0x8C
i2cset -f -y 41 0x2f 0x09 0x8C
i2cset -f -y 41 0x2f 0x0A 0x8C
i2cset -f -y 41 0x2f 0x0B 0x8C
i2cset -f -y 41 0x2f 0x0C 0x8C
i2cset -f -y 41 0x2f 0x0D 0x8C


echo "Probe FAN Device"
i2c_device_add 40 0x20 max31790
i2c_device_add 40 0x2f max31790
i2c_device_add 41 0x20 max31790
i2c_device_add 41 0x2f max31790

echo "Probe ADM1272 Device"
#HSC ADM1272 CONFIG
#Reg 0xD4
i2cset -y -f 38 0x10 0xd4 0x3F1C w
i2cset -y -f 39 0x13 0xd4 0x3F1C w
i2cset -y -f 39 0x1c 0xd4 0x3F1C w
i2c_device_add 38 0x10 adm1272
i2c_device_add 39 0x13 adm1272
i2c_device_add 39 0x1c adm1272


echo "Probe FRU Device"
#ADD eeprom
i2c_device_add 33 0x51 24c64 #MB FRU
i2c_device_add 36 0x52 24c64 #PDBV FRU
i2c_device_add 37 0x51 24c64 #PDBH FRU
i2c_device_add 40 0x56 24c64 #BP0 FRU
i2c_device_add 41 0x56 24c64 #BP1 FRU

i2c_device_add 42 0x50 24c64 #FAN1 FRU
i2c_device_add 43 0x50 24c64 #FAN2 FRU
i2c_device_add 44 0x50 24c64 #FAN3 FRU
i2c_device_add 45 0x50 24c64 #FAN4 FRU
i2c_device_add 46 0x50 24c64 #FAN5 FRU
i2c_device_add 47 0x50 24c64 #FAN6 FRU
i2c_device_add 48 0x50 24c64 #FAN7 FRU
i2c_device_add 49 0x50 24c64 #FAN8 FRU
i2c_device_add 50 0x50 24c64 #FAN9 FRU
i2c_device_add 51 0x50 24c64 #FAN10 FRU
i2c_device_add 52 0x50 24c64 #FAN11 FRU
i2c_device_add 53 0x50 24c64 #FAN12 FRU
i2c_device_add 54 0x50 24c64 #FAN13 FRU
i2c_device_add 55 0x50 24c64 #FAN14 FRU
i2c_device_add 56 0x50 24c64 #FAN15 FRU
i2c_device_add 57 0x50 24c64 #FAN16 FRU

echo "Probe Ex-GPIO device"
i2c_device_add 40 0x21 pca9555
i2c_device_add 41 0x21 pca9555

echo "Probe FAN LED Device"
i2c_device_add 40 0x62 pca9552
i2c_device_add 41 0x62 pca9552


# I/O Expander PCA95552 0xEE
gpio_export_ioexp 40-0021 BP0_FAN0_PRESENT 0
gpio_export_ioexp 40-0021 BP0_FAN1_PRESENT 1
gpio_export_ioexp 40-0021 BP0_FAN2_PRESENT 2
gpio_export_ioexp 40-0021 BP0_FAN3_PRESENT 3
gpio_export_ioexp 40-0021 BP0_FAN4_PRESENT 4
gpio_export_ioexp 40-0021 BP0_FAN5_PRESENT 5
gpio_export_ioexp 40-0021 BP0_FAN6_PRESENT 6
gpio_export_ioexp 40-0021 BP0_FAN7_PRESENT 7

gpio_export_ioexp 41-0021 BP1_FAN0_PRESENT 0
gpio_export_ioexp 41-0021 BP1_FAN1_PRESENT 1
gpio_export_ioexp 41-0021 BP1_FAN2_PRESENT 2
gpio_export_ioexp 41-0021 BP1_FAN3_PRESENT 3
gpio_export_ioexp 41-0021 BP1_FAN4_PRESENT 4
gpio_export_ioexp 41-0021 BP1_FAN5_PRESENT 5
gpio_export_ioexp 41-0021 BP1_FAN6_PRESENT 6
gpio_export_ioexp 41-0021 BP1_FAN7_PRESENT 7
