#!/bin/sh
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

get_sku()
{
  get_uic_location
  uic_id=$?
  get_uic_type
  uic_type=$?

  if [ "$uic_type" = "$UIC_TYPE_5" ]; then
    printf "System: Type5\n"
    if [ "$uic_id" = "$UIC_LOCATION_A" ]; then
      printf "UIC Location: Side A\n"
    elif [ "$uic_id" = "$UIC_LOCATION_B" ]; then
      printf "UIC Location: Side B\n"
    else
      printf "UIC Location: Unknown\n"
    fi
  elif [ "$uic_type" = "$UIC_TYPE_7_HEADNODE" ]; then
    printf "System: Type7 Headnode\n"
  else
    printf "System: Unknown\n"
  fi

  pal_sku=$(((uic_id << UIC_TYPE_SIZE) | uic_type))

  return $pal_sku
}

get_uic_location()
{
  # * UIC_ID: 1=UIC_A; 2=UIC_B
  uic_id=$("$EXPANDERUTIL_CMD" "$NETFN_EXPANDER_REQ" "$CMD_GET_UIC_LOCATION")
  
  return "$uic_id"
}

#             UIC_LOC_TYPE_IN   UIC_RMT_TYPE_IN   SCC_LOC_TYPE_0   SCC_RMT_TYPE_0
#  Type 5                   0                 0                0                0
#  Type 7 Headnode          0                 1                0                1
get_uic_type()
{
  if [ "$(is_chassis_type7)" = "1" ]; then
    uic_type="$UIC_TYPE_7_HEADNODE"
  else
    uic_type="$UIC_TYPE_5"
  fi

  return "$uic_type"
}

get_sku
pal_sku=$?
printf "Platform SKU: %s (" "$pal_sku"

i=0
while [ "${i}" -lt "$PAL_SKU_SIZE" ]
do
  tmp_sku_bit=$(((pal_sku >> i) & 1))
  sku_bit=$tmp_sku_bit$sku_bit
  i=$((i+1))
done

printf "%s" "$sku_bit"
printf ")\n"
printf "<SKU[5:0] = {UIC_ID0, UIC_ID1, UIC_TYPE0, UIC_TYPE1, UIC_TYPE2, UIC_TYPE3}>\n"

exit $pal_sku
