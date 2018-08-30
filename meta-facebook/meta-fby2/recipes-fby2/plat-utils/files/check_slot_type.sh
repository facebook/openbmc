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

ADC_VALUE=(adc12_value adc13_value adc14_value adc15_value)
VOL_ENABLE=(O4 O5 O6 O7)

i=0
sku=0
while [ "${i}" -lt "${#ADC_VALUE[@]}" ]
do
  SLOT_VOL=`cat /sys/devices/platform/ast_adc.0/"${ADC_VALUE[$i]}" | cut -d \  -f 1 | awk '{printf ("%.3f\n",$1)}'`
  SLOT_VOL_ENABLE=${VOL_ENABLE[$i]}
  y=$(($i*2+1))
  if [[ `awk -v a=1.9 -v b=$SLOT_VOL 'BEGIN{print(a>b)?"1":"0"}'` == 1 &&  `awk -v a=$SLOT_VOL -v b=1.5 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
    sku=2
    i2cset -y $y 0x40 0x5 0xa w
  elif [[ `awk -v a=1.4 -v b=$SLOT_VOL 'BEGIN{print(a>b)?"1":"0"}'` == 1 && `awk -v a=$SLOT_VOL -v b=1.0 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
    sku=1
    i2cset -y $y 0x70 0x5 c
    usleep 10000
    i2cset -y $y 0x40 0x5 0xa w
  elif [[ `awk -v a=0.95 -v b=$SLOT_VOL 'BEGIN{print(a>b)?"1":"0"}'` == 1 && `awk -v a=$SLOT_VOL -v b=0.55 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
    sku=4
  else
    sku=0
  fi

  i=$(($i+1))

  if [ $(is_server_prsnt $i) == "0" ]; then
    sku=3
  fi

  # Do not replace slotX type when it is 12V off
  if [ -f "/tmp/slot$i.bin" ]; then
    if [ $(gpio_get_val $SLOT_VOL_ENABLE) == "0" ]; then
      sku=$(get_slot_type $i)
    fi
  fi

  echo "Slot$i Type: $sku"
  echo $sku > /tmp/slot$i.bin

done
