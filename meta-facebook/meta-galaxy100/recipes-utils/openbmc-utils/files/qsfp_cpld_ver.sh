#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
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


lc_qsfp_cpld_ver() {
  card_type=$(weutil  | grep 'Product Name' | awk -F ':' '{print $2}')
  card_type=${card_type,,}

  major_rev=0
  sub_rev=0
  if [ $card_type == 'lc' ] || [ $card_type == 'galaxy-lc' ]; then
    #QSFP CPLD version
    ((val=$(i2cget -f -y 12 0x31 0x3 2> /dev/null | head -n 1)))
    if [ $val -lt 8 ]; then
      old=$((`i2cget -f -y 12 0x74 0x01`))
      if [ $old -eq 9 ] || [ $old -eq 12 ] || [ $old -eq 13 ]; then
        i2cset -f -y 12 0x74 0x01 0x0
      elif [ $old -eq 10 ] || [ $old -eq 14 ] || [ $old -eq 15 ]; then
        i2cset -f -y 12 0x74 0x01 0x1
      elif [ $old -eq 0 ] || [ $old -eq 1 ] || [ $old -eq 5 ]; then
        i2cset -f -y 12 0x74 0x01 0x4
      elif [ $old -eq 2 ] || [ $old -eq 3 ] || [ $old -eq 6 ]; then
        i2cset -f -y 12 0x74 0x01 0x5
      fi
      temp=$((`i2cget -f -y 12 0x39 0x01 2> /dev/null`))
      major_rev=$(($temp & 0x3f))
      sub_rev=$(i2cget -f -y 12 0x39 0x02 2> /dev/null | awk -F "0x" '{print $2}')
    fi
  fi
    printf "%02x,%02d\n" ${major_rev} ${sub_rev}
}

lc_qsfp_cpld_ver
