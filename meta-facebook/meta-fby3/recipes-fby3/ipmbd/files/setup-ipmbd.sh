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

function init_class1_ipmb(){
  for slot_num in $(seq 1 4); do
    bus=$((slot_num-1))
    echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-${bus}/new_device
    runsv /etc/sv/ipmbd_${bus} > /dev/null 2>&1 &
    slot_present=$(gpio_get PRSNT_MB_BMC_SLOT${slot_num}_BB_N)
    if [[ "$slot_present" == "1" ]]; then
      sv stop ipmbd_${bus}
    else
      enable_server_i2c_bus ${slot_num}
    fi
  done  
}

function init_class2_ipmb(){
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-0/new_device
  runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &
}

echo -n "Starting IPMB Rx/Tx Daemon.."

ulimit -q 1024000

bmc_location=$(get_bmc_board_id)
if [ $bmc_location -eq 9 ]; then
  #The BMC of class2
  init_class2_ipmb
elif [ $bmc_location -eq 14 ]; then
  #The BMC of class1
  init_class1_ipmb
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "Done."
