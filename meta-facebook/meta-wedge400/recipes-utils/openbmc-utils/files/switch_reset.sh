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

#shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

if [ "$(wedge_board_type)" != "1" ]; then
    echo "$(basename "$0") only support on Wedge400C"
    exit 1
fi

SMB_CPLD_I2C_BUS="12"
SMB_CPLD_I2C_ADDR="0x3e"

i2c_wr() {
    i2cset "$@"; sleep 0.01;
}

# Wedge400-C GB switch reset commands collection
gb_reset() {
    if [ "$1" = "cycle" ];then
        # gb power off
        i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x42 0x0e
        sleep 0.5
        # gb power on
        i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x42 0x0f
        sleep 1
    fi

    # [3] mux_ps=1, ps=1, scan=0, tri_l=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xd
    # [0] a_rstn=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x05 0xfe
    # [0] tri_l=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xc
    # [1] scan_mode=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xe
    # [1] scan_mode=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xc
    # [0] tri_l=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xd
    # [0] a_rstn=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x05 0xff
    # [0] a_rstn=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x05 0xfe
    # [2] ps=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0x9
    # [2] ps=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0xd
    sleep 0.5
    # [0] a_rstn=1
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x05 0xff
    # [3] mux_ps=0
    i2c_wr -f -y $SMB_CPLD_I2C_BUS $SMB_CPLD_I2C_ADDR 0x60 0x5
}

case "$1" in
    "--help" | "?" | "-h")
        program=$(basename "$0")
        echo "Usage: switch chip reset command"
        echo "  <$program> only reset switch without switch power cycle"
        echo "  <$program cycle> to reset switch after switch power cycle"
    ;;

    *)
        shift
        gb_reset "$1"
    ;;
esac
