#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
#

. /usr/local/fbpackages/utils/ast-functions

kernel_ver=$(get_kernel_ver)

if [ "$#" -eq 1 ]; then
  case $1 in
    slot1)
      SLOT=(1)
      VOL_EN=(O4)
      VOL_EN_NAME=(P12V_STBY_SLOT1_EN)
      if [ $kernel_ver == 5 ]; then
        ADC_VALUE=(in12_input)
      else
        ADC_VALUE=(adc12_value)
      fi
      ;;
    slot2)
      SLOT=(2)
      VOL_EN=(O5)
      VOL_EN_NAME=(P12V_STBY_SLOT1_EN)
      if [ $kernel_ver == 5 ]; then
        ADC_VALUE=(in13_input)
      else
        ADC_VALUE=(adc13_value)
      fi
      ;;
    slot3)
      SLOT=(3)
      VOL_EN=(O6)
      VOL_EN_NAME=(P12V_STBY_SLOT1_EN)
      if [ $kernel_ver == 5 ]; then
        ADC_VALUE=(in14_input)
      else
        ADC_VALUE=(adc14_value)
      fi
      ;;
    slot4)
      SLOT=(4)
      VOL_EN=(O7)
      VOL_EN_NAME=(P12V_STBY_SLOT1_EN)
      if [ $kernel_ver == 5 ]; then
        ADC_VALUE=(in15_input)
      else
        ADC_VALUE=(adc15_value)
      fi
      ;;
    *)
      exit 1
      ;;
  esac
else
  SLOT=(1 2 3 4)
  VOL_EN=(O4 O5 O6 O7)
  VOL_EN_NAME=(P12V_STBY_SLOT1_EN P12V_STBY_SLOT2_EN P12V_STBY_SLOT3_EN P12V_STBY_SLOT4_EN)
  if [ $kernel_ver == 5 ]; then
      ADC_VALUE=(in12_input in13_input in14_input in15_input)
  else
      ADC_VALUE=(adc12_value adc13_value adc14_value adc15_value)
  fi
fi


sku=0
for (( i=0; i<${#SLOT[@]}; i++ )); do
  slot_id=${SLOT[$i]}
  if [ $(is_server_prsnt $slot_id) != "1" ]; then
    sku=3
  elif [ $(gpio_get ${VOL_EN_NAME[$i]} ${VOL_EN[$i]}) == "1" ]; then
    if [ $kernel_ver == 5 ]; then
      slot_vol=`cat /sys/devices/platform/iio-hwmon/hwmon/hwmon*/"${ADC_VALUE[$i]}" | cut -d \  -f 1 | awk '{printf ("%.3f\n",$1)}'`
    else
      slot_vol=`cat /sys/devices/platform/ast_adc.0/"${ADC_VALUE[$i]}" | cut -d \  -f 1 | awk '{printf ("%.3f\n",$1)}'`
    fi
    
    if [[ `awk -v a=1.9 -v b=$slot_vol 'BEGIN{print(a>b)?"1":"0"}'` == 1 &&  `awk -v a=$slot_vol -v b=1.5 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
      sku=2
      y=$(($slot_id*2-1))
      i2cset -y $y 0x40 0x5 0xa w
    elif [[ `awk -v a=1.4 -v b=$slot_vol 'BEGIN{print(a>b)?"1":"0"}'` == 1 && `awk -v a=$slot_vol -v b=1.0 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
      sku=1
      y=$(($slot_id*2-1))
      i2cset -y $y 0x70 0x5 c
      usleep 10000
      i2cset -y $y 0x40 0x5 0xa w
    elif [[ `awk -v a=0.95 -v b=$slot_vol 'BEGIN{print(a>b)?"1":"0"}'` == 1 && `awk -v a=$slot_vol -v b=0.55 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
      sku=4
    else
      sku=0
    fi
  elif [ ! -f "/tmp/slot$slot_id.bin" ]; then
    sku=3
  else
    continue
  fi

  echo "Slot$slot_id Type: $sku"
  echo $sku > /tmp/slot$slot_id.bin
done
