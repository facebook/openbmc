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

function init_class2_ipmb(){
  runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &
}

function init_class1_ipmb(){
  runsv /etc/sv/ipmbd_0 > /dev/null 2>&1 &
  runsv /etc/sv/ipmbd_1 > /dev/null 2>&1 &
  runsv /etc/sv/ipmbd_2 > /dev/null 2>&1 &
  runsv /etc/sv/ipmbd_3 > /dev/null 2>&1 &
#  runsv /etc/sv/ipmbd_9 > /dev/null 2>&1 &

  if [ $(is_server_prsnt 0) == "0" ]; then
    sv stop ipmbd_0
  else
    en_server_12V_power 0
    en_server_i2c_bus 0
  fi

  if [ $(is_server_prsnt 1) == "0" ]; then
    sv stop ipmbd_1
  else
    en_server_12V_power 1
    en_server_i2c_bus 1
  fi

  if [ $(is_server_prsnt 2) == "0" ]; then
    sv stop ipmbd_2
  else
    en_server_12V_power 2
    en_server_i2c_bus 2
  fi

  if [ $(is_server_prsnt 3) == "0" ]; then
    sv stop ipmbd_3
  else
    en_server_12V_power 3
    en_server_i2c_bus 3
  fi
}

echo "Starting IPMB Rx/Tx Daemon"

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
