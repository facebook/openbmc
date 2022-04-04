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

if [ "$#" -eq 1 ]; then
  case $1 in
    slot1)
      SLOT=(1)
      ;;
    slot2)
      SLOT=(2)
      ;;
    slot3)
      SLOT=(3)
      ;;
    slot4)
      SLOT=(4)
      ;;
    *)
      exit 1
      ;;
  esac
else
  SLOT=(1 2 3 4)
fi

TL_PRODUCT_ID0=39
TL_PRODUCT_ID1=30
ND_PRODUCT_ID0=44
ND_PRODUCT_ID1=4E

max_retry=3

for (( i=0; i<${#SLOT[@]}; i++ )); do
  slot_id=${SLOT[$i]}
  slot_12v=$(is_slot_12v_on $slot_id)
  if [ $(is_server_prsnt $slot_id) != "1" ]; then
    server_type=3
  elif [[ "$slot_12v" == "1" && $(get_slot_type $slot_id) == "0" ]]; then
    j=0
    while [ ${j} -lt ${max_retry} ]; do
      # Use standard IPMI command 'get-device-id' to read product id of server board
      output=$(/usr/bin/bic-util slot$slot_id 0x18 0x01)
      # if the command fails and the number of retry times reach max retry, continue to next slot
      if [ $(echo $output | wc -c) == 45 ]; then
        product_id_l=`echo "$output" | cut -c 28-29`
        product_id_h=`echo "$output" | cut -c 31-32`
        if [[ "$product_id_l" == "$TL_PRODUCT_ID0" && "$product_id_h" == "$TL_PRODUCT_ID1" ]]; then
          #TL
          server_type=0
        elif [[ "$product_id_l" == "$ND_PRODUCT_ID0" && "$product_id_h" == "$ND_PRODUCT_ID1" ]]; then
          #ND
          server_type=4
          RES=$(/usr/bin/bic-util slot$slot_id 0xE0 0x2A 0x15 0xA0 0x00 0x01)
          postcode=$(echo $RES| awk '{print $7$6$5$4;}')
          # Since server may not power on, skip if post code empty
          if [ -n "$postcode" ]; then
            /usr/bin/kv set "slot${slot_id}_last_postcode" "$postcode" 2> /dev/null
          fi
        else
          #unknown
          server_type=3
        fi
        break
      fi
      sleep 1
      j=$(($j+1))
    done
  elif [[ "$slot_12v" == "1" || ! -f "/tmp/server_type$slot_id.bin" ]]; then
    server_type=3
  else
    continue
  fi

  echo "Slot$slot_id Server Type: $server_type"
  echo $server_type > /tmp/server_type$slot_id.bin
done
