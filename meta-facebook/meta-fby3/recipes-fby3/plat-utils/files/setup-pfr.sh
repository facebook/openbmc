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
RCVY_CNT=0x0
PANIC_CNT=0x0
ret=0
# NIC EXP BMC
if [ $BOARD_ID -eq 9 ]; then
    for i in {1..3}; do
        /usr/sbin/i2craw 9 0x38 -w "0x0F 0x09" > /dev/null
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

    for i in {1..3}; do
        RCVY_CNT=$(/usr/sbin/i2cget -y 9 0x38 0x04)
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

    for i in {1..3}; do
        PANIC_CNT=$(/usr/sbin/i2cget -y 9 0x38 0x06)
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

# Baseboard BMC
elif [ $BOARD_ID -eq 14 ] || [ $BOARD_ID -eq 7 ]; then
    for i in {1..3}; do
        /usr/sbin/i2craw 12 0x38 -w "0x0F 0x09" > /dev/null
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

    for i in {1..3}; do
        RCVY_CNT=$(/usr/sbin/i2cget -y 12 0x38 0x04)
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

    for i in {1..3}; do
        PANIC_CNT=$(/usr/sbin/i2cget -y 12 0x38 0x06)
        ret=$?
        if [[ "$ret" == "0" ]]; then
            break
        fi
    done

else
  echo "Is board id correct(id=$BOARD_ID)?"
fi

echo "Check PFR MailBox Status..."
if [[ "$RCVY_CNT" -ne "0x0" ]]; then
    logger -t "pfr" -p daemon.crit "BMC, PFR - Recovery count: $((RCVY_CNT))"
fi


if [[ "$PANIC_CNT" -ne "0x0" ]]; then
    logger -t "pfr" -p daemon.crit "BMC, PFR - Panic event count: $((PANIC_CNT))"
fi
