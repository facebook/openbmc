#!/bin/bash
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

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# SWPLD1_BUS="7"
# SWPLD1_ADDR="0x32"
# RST1="0x06"
# RST2="0x07"
# RST3="0x08"

driver_echo() {
    echo "$@"; sleep 0.02;
}

# AGC032A switch reset commands collection
gb_reset() {
    # i2c_wr -f -y $SWPLD1_BUS $SWPLD1_ADDR $RST1 0xFC
    driver_echo 0 > $MAC_RST_SYSFS
    # i2c_wr -f -y $SWPLD1_BUS $SWPLD1_ADDR $RST1 0xFF
    driver_echo 0 > $MAC_PCIE_RST_SYSFS

    driver_echo 1 > $MAC_PCIE_RST_SYSFS
    driver_echo 1 > $MAC_RST_SYSFS
}

case "$1" in
    "--help" | "-h")
        echo "Usage: switch chip reset command"
        echo "only reset switch without switch power cycle"
    ;;

    *)
        shift
        gb_reset "$1"
    ;;
esac
