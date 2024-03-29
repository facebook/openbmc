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

# shellcheck disable=SC1091,SC2039
. /usr/local/fbpackages/utils/ast-functions

echo -n "Starting MCTP Rx/Tx Daemon.."

init_class1_mctp() {
  for slot_num in {1..4}; do
    bus=$((slot_num))
    runsv /etc/sv/mctpd_${bus} > /dev/null 2>&1 &
    if [ "$slot_num" -eq 1 ] || [ "$slot_num" -eq 3 ]; then
      slot_present=$(gpio_get PRSNT_MB_BMC_SLOT"${slot_num}"_BB_N)
    else
      slot_present=$(gpio_get PRSNT_MB_SLOT"${slot_num}"_BB_N)
    fi
    if [ "$slot_present" = "1" ]; then
      usleep 50000
      sv stop mctpd_${bus}
      disable_server_12V_power "${slot_num}"
    else
      enable_server_i2c_bus "${slot_num}"
    fi
  done
}

init_class2_mctp() {
  runsv /etc/sv/mctpd_1 > /dev/null 2>&1 &
}

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_mctp
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_mctp
else
  echo -n "setup_mctpd: Is board id correct(id=$bmc_location)?..."
fi

echo slave-mqueue 0x1010 > /sys/bus/i2c/devices/i2c-8/new_device

runsv /etc/sv/mctpd_8 > /dev/null 2>&1 &

echo "done."

