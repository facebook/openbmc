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

RC_PRODUCT_ID0=43
RC_PRODUCT_ID1=52
EP_PRODUCT_ID0=50
EP_PRODUCT_ID1=45
TL_PRODUCT_ID0=39
TL_PRODUCT_ID1=30

get_server_sku()
{
   j=0
   max_retry=3
   server_type=0
   for i in 1 2 3 4
   do
     slot_12v=$(is_slot_12v_on $i)
     if [[ $(is_server_prsnt $i) == "1" && "$slot_12v" == "1" && $(get_slot_type $i) == "0" ]] ; then
       tmp_server_type=3
       j=0
       while [ ${j} -lt ${max_retry} ]
       do
         # Use standard IPMI command 'get-device-id' to read product id of server board
         output=$(/usr/bin/bic-util slot$i 0x18 0x01)
         # if the command fails and the number of retry times reach max retry, continue to next slot
         if [ $(echo $output | wc -c) == 45 ] ; then
           product_id_l=`echo "$output" | cut -c 28-29`
           product_id_h=`echo "$output" | cut -c 31-32`
           if [[ "$product_id_l" == "$RC_PRODUCT_ID0" && "$product_id_h" == "$RC_PRODUCT_ID1" ]] ; then
             #RC
             tmp_server_type=1
           elif [[ "$product_id_l" == "$EP_PRODUCT_ID0" && "$product_id_h" == "$EP_PRODUCT_ID1" ]] ; then
             #EP
             tmp_server_type=2
           elif [[ "$product_id_l" == "$TL_PRODUCT_ID0" && "$product_id_h" == "$TL_PRODUCT_ID1" ]] ; then
             #TL
             tmp_server_type=0
           else
             #unknown
             tmp_server_type=3
           fi
           break
         fi
         sleep 1
         j=$(($j+1))
       done
     else
       tmp_server_type=3

       # Do not replace slotX server type when it is 12V off
       if [[ "$slot_12v" == "0" && -f /tmp/server_type.bin ]] ; then
         tmp_server_type=$(get_server_type $i)
       fi
     fi
     server_type=$(($(($tmp_server_type << $((($i-1) * 2)))) + $server_type))
   done

   return $server_type
}

get_server_sku
SERVER_TYPE=$?
echo "Server Type: $SERVER_TYPE "
echo "<SERVER_TYPE[7:0] = {SLOT4, SLOT3, SLOT2, SLOT1}>"

exit $SERVER_TYPE
