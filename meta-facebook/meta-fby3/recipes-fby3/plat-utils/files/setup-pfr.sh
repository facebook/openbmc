#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin

. /usr/local/fbpackages/utils/ast-functions

# CHKPT_COMPLETE

BOARD_ID=$(get_bmc_board_id)
# NIC EXP BMC
if [ $BOARD_ID -eq 9 ]; then
    /usr/sbin/i2craw 9 0x38 -w "0x0F 0x09"
# Baseboard BMC
elif [ $BOARD_ID -eq 14 ] || [ $BOARD_ID -eq 7 ]; then
    /usr/sbin/i2craw 12 0x38 -w "0x0F 0x09"
else
  echo "Is board id correct(id=$BOARD_ID)?"
fi
