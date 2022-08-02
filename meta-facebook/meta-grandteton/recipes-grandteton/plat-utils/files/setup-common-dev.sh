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

echo "Probe MB MUX"
rebind_i2c_dev 1 70 pca954x
rebind_i2c_dev 5 70 pca954x
rebind_i2c_dev 8 72 pca954x

echo "Probe PDBH MUX"
rebind_i2c_dev 37 71 pca954x

echo "Probe BP Mux"
rebind_i2c_dev 40 70 pca954x
rebind_i2c_dev 41 70 pca954x

echo "Probe TEMP Device"
i2c_device_add 21 0x48 stlm75
i2c_device_add 22 0x48 stlm75
i2c_device_add 23 0x48 stlm75
i2c_device_add 24 0x48 stlm75

echo Probe ADM128D
i2cset -f -y 20 0x1d 0x0b 0x02
i2c_device_add 20 0x1d adc128d818

#FAN Max 31790
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

echo Probe Ex-GPIO device
i2c_device_add 29 0x74 pca9539 #MB Expender
i2c_device_add 32 0x76 pca9539 #BIC Expender
i2c_device_add 40 0x21 pca9555
i2c_device_add 41 0x21 pca9555

echo Probe FAN LED Device
rebind_i2c_dev 40 62 leds-pca955x
rebind_i2c_dev 41 62 leds-pca955x

echo Probe PDBV Birck Device
i2c_device_add 38 0x69 pmbus
i2c_device_add 38 0x6a pmbus
i2c_device_add 38 0x6b pmbus

