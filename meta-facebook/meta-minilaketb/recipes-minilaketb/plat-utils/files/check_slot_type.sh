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

ADC_VALUE=(adc12_value)
VOL_ENABLE=(O4)

get_sku()
{
   i=0
   sku=0
   while [ "${i}" -lt "${#ADC_VALUE[@]}" ]
   do
     SLOT_VOL=`cat /sys/devices/platform/ast_adc.0/"${ADC_VALUE[$i]}" | cut -d \  -f 1 | awk '{printf ("%.3f\n",$1)}'`
     y=$(($i*2+1))
     if [[ `awk -v a=1.9 -v b=$SLOT_VOL 'BEGIN{print(a>b)?"1":"0"}'` == 1 &&  `awk -v a=$SLOT_VOL -v b=1.5 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
        tmp_sku=2
        i2cset -y $y 0x40 0x5 0xa w
     elif [[ `awk -v a=1.4 -v b=$SLOT_VOL 'BEGIN{print(a>b)?"1":"0"}'` == 1 && `awk -v a=$SLOT_VOL -v b=1.0 'BEGIN{print(a>b)?"1":"0"}'` == 1 ]]; then
        tmp_sku=1
        i2cset -y $y 0x70 0x5 c
        usleep 10000
        i2cset -y $y 0x40 0x5 0xa w
     else
        tmp_sku=0
     fi

     if [ $(is_server_prsnt $(($i+1))) == "0" ]; then
        tmp_sku=3
     fi

     # Do not replace slotX type when it is 12V off
     if [ -f /tmp/slot.bin ]; then
        if [ $(gpio_get_val ${VOL_ENABLE[$i]}) == "0" ]; then
           tmp_sku=$(get_slot_type $(($i+1)))
        fi
     fi

     sku=$(($(($tmp_sku << $(($i * 2)))) + $sku))
     i=$(($i+1))
   done

   return $sku
}

get_sku
SLOT_TYPE=$?
echo "Slot Type: $SLOT_TYPE "
echo "<SLOT_TYPE[7:0] = {SLOT4, SLOT3, SLOT2, SLOT1}>"

exit $SLOT_TYPE
