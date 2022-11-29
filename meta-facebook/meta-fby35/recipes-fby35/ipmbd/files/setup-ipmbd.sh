#! /bin/bash
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

# shellcheck source=meta-facebook/meta-fby35/recipes-fby35/plat-utils/files/ast-functions
. /usr/local/fbpackages/utils/ast-functions

init_class1_ipmb() {
  for slot_num in {1..4}; do
    bus=$((slot_num-1))
    echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-${bus}/new_device
    runsv /etc/sv/ipmbd_${bus} > /dev/null 2>&1 &
    if [ $slot_num -eq 1 ] || [ $slot_num -eq 3 ]; then
      slot_present=$(gpio_get_value PRSNT_MB_BMC_SLOT${slot_num}_BB_N)
    else
      slot_present=$(gpio_get_value PRSNT_MB_SLOT${slot_num}_BB_N)
    fi
    if [ "$slot_present" = "1" ]; then
      usleep 50000
      sv stop ipmbd_${bus}
      disable_server_12V_power ${slot_num}
    else
      enable_server_i2c_bus ${slot_num}
    fi
  done

  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-9/new_device
  runsv /etc/sv/ipmbd_9 > /dev/null 2>&1 &

  # add BMC slave address to NIC I2C bus for MCTP
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-8/new_device
}

init_class2_ipmb() {
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-0/new_device
  runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &

  # add BMC slave address to NIC I2C bus for MCTP
  echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-8/new_device
}

echo -n "Starting IPMB Rx/Tx Daemon.."
sleep 10
ulimit -q 1024000

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_ipmb
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_ipmb
else
  echo -n "Is board id correct(id=$bmc_location)?..."
fi

echo "Done."
