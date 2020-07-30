#!/bin/bash
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
# shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

COM_E_PWRGD=$(head -n 1 "${SCMCPLD_SYSFS_DIR}/come_pwr_ok_status")

usage(){
    program=$(basename "$0")
    echo "Usage:"
    echo "     $program led"
}

if [ $# -ne 1 ]; then
    usage
    exit 1
fi

# When COM-e power is not ready, it doesn't work to set PHY register.
# Therefore, waiting for COM-e power ready, then start to set PHY register.
while [ "$COM_E_PWRGD" = "0x0" ];
do
    COM_E_PWRGD=$(head -n 1 "${SCMCPLD_SYSFS_DIR}/come_pwr_ok_status")
    sleep 5
done

if [ "$1" = "led" ]; then
    echo "Wait a few seconds to setup management port LED..."
    # refer to BCM54616S Section 5, register 1C access
    # Register 1C (Shadow 00010): Spare Control 1
    # Bit0: 1: Enable link LED mode.
    mdio-util -m 2 -p 0x0e -w 0x1c -d 0x8801
    # Register 1C (Shadow 01110): LED Selector 2
    # Bit7~4: LED4 Selector, b0011: Activity LED 
    # Bit3~0: LED4 Selector, b0101: Slave
    mdio-util -m 2 -p 0x0e -w 0x1c -d 0xb435
    echo "Done!"
else
    usage
    exit 1
fi
