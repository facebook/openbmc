#! /bin/sh
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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

### BEGIN INIT INFO
# Provides:          ipmbd
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:      0 6
# Short-Description: Provides ipmb message tx/rx service
#
### END INIT INFO

. /usr/local/fbpackages/utils/ast-functions

SPB_FRU_FILE="/tmp/fruid_spb.bin"
SPB_TYPE="/tmp/spb_type"

echo "Starting IPMB Rx/Tx Daemon"

kernel_ver=$(get_kernel_ver)
if [ $kernel_ver == 5 ]; then
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-1/new_device
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-3/new_device
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-5/new_device
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-7/new_device
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-13/new_device
fi

ulimit -q 1024000
runsv /etc/sv/ipmbd_1 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_3 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_5 > /dev/null 2>&1 &
runsv /etc/sv/ipmbd_7 > /dev/null 2>&1 &

#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 1) == "1" && $(get_slot_type 1) != "0" && $(get_slot_type 1) != "4" ]]; then
  sv stop ipmbd_1
elif [ $(is_server_prsnt 1) == "0" ]; then
  sv stop ipmbd_1
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 2) == "1" && $(get_slot_type 2) != "0" && $(get_slot_type 2) != "4" ]]; then
  sv stop ipmbd_3
elif [ $(is_server_prsnt 2) == "0" ]; then
  sv stop ipmbd_3
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 3) == "1" && $(get_slot_type 3) != "0" && $(get_slot_type 3) != "4" ]]; then
  sv stop ipmbd_5
elif [ $(is_server_prsnt 3) == "0" ]; then
  sv stop ipmbd_5
fi
#Get slot type (0:TwinLakes, 1:Crace Flat, 2:Glacier Point 3:Empty Slot)
#get_slot_type is to get slot type to check if the slot type is server
if [[ $(is_server_prsnt 4) == "1" && $(get_slot_type 4) != "0" && $(get_slot_type 4) != "4" ]]; then
  sv stop ipmbd_7
elif [ $(is_server_prsnt 4) == "0" ]; then
  sv stop ipmbd_7
fi

if [ -f $SPB_FRU_FILE ]; then
  str=$(/usr/local/bin/fruid-util spb | grep -i "Robinson Creek")
  if [ "$str" != "" ]; then    #RC Baseboard
    echo 1 > $SPB_TYPE
  else
    echo 0 > $SPB_TYPE
  fi
else
  echo 0 > $SPB_TYPE
fi

runsv /etc/sv/ipmbd_13 > /dev/null 2>&1 &
