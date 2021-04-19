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

# Get platform SKU
sh /usr/local/bin/check_pal_sku.sh > /dev/NULL
pal_sku=$?

#Set pal_sku value to key value of system_info
"$CFGUTIL_CMD" "$KEY_SYSTEM_INFO" "$pal_sku"
ret=$?
if [ "$ret" != "0" ]; then
  logger -s -p user.warn -t setup-platform "Failed to set key value of $KEY_SYSTEM_INFO"
  exit 1
fi

# Record the platform SKU in syslog
uic_type=$(($pal_sku & $UIC_TYPE_MASK))
uic_id=$(($(($pal_sku >> $UIC_TYPE_SIZE)) & $UIC_LOCATION_MASK))

if [ "$uic_type" = "$UIC_TYPE_5" ]; then
  if [ "$uic_id" = "$UIC_LOCATION_A" ]; then
    UIC_LOCAL="side A"
  elif [ "$uic_id" = "$UIC_LOCATION_B" ]; then
    UIC_LOCAL="side B"
  else
    UIC_LOCAL="unknown"
  fi
  logger -s -p user.info -t setup-platform "System: type 5, UIC location: $UIC_LOCAL"
elif [ "$uic_type" = "$UIC_TYPE_7_HEADNODE" ]; then
  logger -s -p user.info -t setup-platform "System: type 7"
else
  logger -s -p user.info -t setup-platform "System: unknown"
fi
