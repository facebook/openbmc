#!/bin/bash
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

do_fscd_configure () {
  default_fsc_config="/etc/fsc-config.json"
  hgx_ppn=$(/usr/local/bin/fruid-util hgx | grep 'Product Part Number' | grep -o '[0-9A-Za-z-]*$')

  rm ${default_fsc_config}
  if [[ $hgx_ppn == "" ]]; then
    # If hgx not ready
    echo probe 700w config
    ln -s /etc/fsc-config-700w.json ${default_fsc_config}
    return 1
  elif [[ $hgx_ppn == "935-24287-A510-300" ]]; then
    # If hgx is 500w
    echo probe 500w config
    ln -s /etc/fsc-config-500w.json ${default_fsc_config}
    return 0
  else
    # If hgx is 700w
    echo probe 700w config
    ln -s /etc/fsc-config-700w.json ${default_fsc_config}
    return 0
  fi
}

for retry in {1..20};
do
    bp2_sensor238=$(kv get fan_bp2_sensor238)
    if [ $? -ne 0 ]; then
        sleep 3
    else
        break
    fi
done


#check M1 test board
tb=$(kv get m1_test_board)

if [ "$tb" -eq 0 ]; then
  echo "Check Fan Present "
  val=$(i2cget -f -y 40 0x21 0x00)
  fan_present1=$(("$val"))

  val=$(i2cget -f -y 41 0x21 0x00)
  fan_present2=$(("$val"))

  if [ "$fan_present1" -eq 255 ] && [ "$fan_present2" -eq 255 ]; then
    echo "Don't enable fscd due to fan not present"
  else
    echo "Start fscd"
    do_fscd_configure
    result=$?
    runsv /etc/sv/fscd > /dev/null 2>&1 &

    if [[ $result == 1 ]]; then
      while true; do
        if [[ $result == 0 ]]; then
          sv restart fscd
          break
        else
          sleep 5
          do_fscd_configure
          result=$?
        fi
      done
    fi
  fi
fi
echo "done."
