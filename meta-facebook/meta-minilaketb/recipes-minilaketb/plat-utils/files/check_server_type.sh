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

get_server_sku()
{
   j=0
   max_retry=3
   server_type=0
   for i in 1 2 3 4
   do
     if [[ $(is_server_prsnt $i) == "1" && $(get_slot_type $i) == "0" ]] ; then
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
           if [[ "$product_id_l" == 43 && "$product_id_h" == 52 ]] ; then
             #RC
             tmp_server_type=1
           elif [[ "$product_id_l" == 50 && "$product_id_h" == 45 ]] ; then
             #EP
             tmp_server_type=2
           elif [[ "$product_id_l" == 39 && "$product_id_h" == 30 ]] ; then
             #TL
             tmp_server_type=0
           else
             #unknown
             tmp_server_type=3
           fi
           break
         fi
         j=$(($j+1))
       done
     else
       tmp_server_type=3
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
