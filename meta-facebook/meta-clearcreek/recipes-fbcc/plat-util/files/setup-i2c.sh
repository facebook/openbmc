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
. /usr/local/fbpackages/utils/ast-functions

KVSET_CMD=/usr/bin/kv

echo "Setup TCA9539 input/output pin"
i2cset -y -f 5 0x77 0x06 0x00
i2cset -y -f 5 0x77 0x07 0x0f
i2cset -y -f 5 0x77 0x02 0x0f

#Setup BB MAXIM ADC
gp_board_id0=$(gpio_get BOARD_ID0 GPION5)
gp_board_id1=$(gpio_get BOARD_ID1 GPION6)
if [ $gp_board_id0 -eq 1 ] && [ $gp_board_id1 -eq 0 ]; then
  i2c_device_add 23 0x33 max11615
  i2c_device_add 23 0x35 max11617
fi

#Setup TMP75 for carrier board
gp_carrier1_prsnt=$(gpio_get NVME_0_1_PRSNTB_R_N)
if [ $gp_carrier1_prsnt -eq 0 ]; then
  i2c_device_add 21 0x48 tmp75
  i2c_device_add 21 0x49 tmp75
  i2cset -y -f 21 0x1d 0x0b 0x2
  i2cset -y -f 21 0x1f 0x0b 0x2
  i2c_device_add 21 0x1d adc128d818
  i2c_device_add 21 0x1f adc128d818
fi

gp_carrier2_prsnt=$(gpio_get NVME_2_3_PRSNTB_R_N)
if [ $gp_carrier2_prsnt -eq 0 ]; then
  i2c_device_add 22 0x48 tmp75
  i2c_device_add 22 0x49 tmp75
  i2cset -y -f 22 0x1d 0x0b 0x2
  i2cset -y -f 22 0x1f 0x0b 0x2
  i2c_device_add 22 0x1d adc128d818
  i2c_device_add 22 0x1f adc128d818
fi

#Set PWR_AVR to 128 samples
i2cset -y -f 5 0x11 0xd4 0x3F1E w

#Set FIO LED
i2cset -y -f 0x08 0x77 0x7 0x3F
i2cset -y -f 0x08 0x77 0x3 0x80

#Check carrier#0 type
retry=0
while [ $retry -lt 3 ]
do
carrier0=`i2ctransfer -f -y 23 w2@0x17 0x00 0x05 r1`
if [ $carrier0 = "0xcc" ]; then
    $KVSET_CMD set "carrier_0" "m.2"
    i2cset -y -f 21 0x77 0x2 0xFF
    echo "Carrier#0 Type is m.2"
    #Set M.2 carrier INA219 Calibration register
    i2cset -y -f 21 0x45 5 0xC800 w
    break
elif [ $carrier0 = "0x22" ] || [ $carrier0 = "0x44" ]; then
    $KVSET_CMD set "carrier_0" "e1.s"
    i2cset -y -f 21 0x77 0x02 0x00
    echo "Carrier#0 Type is e1.s"
    break
elif [ $carrier0 = "0x33" ]; then
    $KVSET_CMD set "carrier_0" "e1.s_v2"
    i2cset -y -f 21 0x77 0x02 0x00
    echo "Carrier#0 Type is e1.s_v2"
    break
elif [ $carrier0 = "0xdd" ]; then
    $KVSET_CMD set "carrier_0" "m.2_v2"
    i2cset -y -f 21 0x77 0x02 0xFF
    echo "Carrier#0 Type is m.2_v2"
    #Set M.2 carrier INA219 Calibration register
    i2cset -y -f 21 0x45 5 0xC800 w
    break
else
    retry=$(($retry+1))
fi
done

#Check carrier#1 type
retry=0
while [ $retry -lt 3 ]
do
carrier1=`i2ctransfer -f -y 23 w2@0x17 0x00 0x06 r1`
if [ $carrier1 = "0xcc" ]; then
    $KVSET_CMD set "carrier_1" "m.2"
    i2cset -y -f 22 0x77 0x2 0xFF
    echo "Carrier#1 Type is m.2"
    #Set M.2 carrier INA219 Calibration register
    i2cset -y -f 22 0x45 5 0xC800 w
    break
elif [ $carrier1 = "0x22" ] || [ $carrier1 = "0x44" ]; then
    $KVSET_CMD set "carrier_1" "e1.s"
    i2cset -y -f 22 0x77 0x02 0x00
    echo "Carrier#1 Type is e1.s"
    break
elif [ $carrier1 = "0x33" ]; then
    $KVSET_CMD set "carrier_1" "e1.s_v2"
    i2cset -y -f 22 0x77 0x02 0x00
    echo "Carrier#1 Type is e1.s_v2"
    break
elif [ $carrier1 = "0xdd" ]; then
    $KVSET_CMD set "carrier_1" "m.2_v2"
    i2cset -y -f 22 0x77 0x02 0xFF
    echo "Carrier#1 Type is m.2_v2"
    #Set M.2 carrier INA219 Calibration register
    i2cset -y -f 22 0x45 5 0xC800 w
    break
else
    retry=$(($retry+1))
fi
done

# Mark driver loaded
echo > /tmp/driver_probed
