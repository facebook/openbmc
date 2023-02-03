#! /bin/sh
#
# Copyright 2022-present Facebook. All Rights Reserved
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

init_class1_pldm() {
  for slot_num in {1..4}; do
    bus=$((slot_num))
    runsv /etc/sv/pldmd_${bus} > /dev/null 2>&1 &
  done
}

init_class2_pldm() {
  runsv /etc/sv/pldmd_1 > /dev/null 2>&1 &
}

echo -n "Starting PLDMD Rx/Tx Daemon.."

ulimit -q 1024000

bmc_location=$(get_bmc_board_id)
if [ "$bmc_location" -eq "$BMC_ID_CLASS2" ]; then
  #The BMC of class2
  init_class2_pldm
elif [ "$bmc_location" -eq "$BMC_ID_CLASS1" ]; then
  #The BMC of class1
  init_class1_pldm
else
  echo -n "setup-pldmd: Is board id correct(id=$bmc_location)?..."
fi

echo "done."

