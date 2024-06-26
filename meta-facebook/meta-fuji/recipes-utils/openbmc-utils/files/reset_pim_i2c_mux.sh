#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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
# The script resets the I2C Muxes between BMC SoC and the I2C client
# devices (such as DOM FPGAs) on PIMs.
# It's used to recover i2c-11 when a certain I2C device/mux locks the
# bus forever, which prevents access to all the I2C devices on the PIMs.
# Total 9 I2C Muxes are involved:
#   -> 11-0077, first level mux on SMB
#      -> 40-0076 on PIM 1, connected to 11-0077, channel-0
#      -> 41-0076 on PIM 2, connected to 11-0077, channel-1
#      -> 42-0076 on PIM 3, connected to 11-0077, channel-2
#      -> 43-0076 on PIM 4, connected to 11-0077, channel-3
#      -> 44-0076 on PIM 5, connected to 11-0077, channel-4
#      -> 45-0076 on PIM 6, connected to 11-0077, channel-5
#      -> 46-0076 on PIM 7, connected to 11-0077, channel-6
#      -> 47-0076 on PIM 8, connected to 11-0077, channel-7
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

usage() {
    echo "Usage: $0 <all|smb|pim1..8>"
    echo "For example:"
    echo "   To reset all (SMB + PIMs): $0 all"
    echo "   To reset I2C mux on PIM7: $0 pim7"
    echo "   To reset I2C mux on SMB: $0 smb"
    echo ""
}

# Reset the first level PIM Mux (11-0077, on SMB).
reset_smb_mux() {
    echo "reset smb-pim i2c mux.."
    echo 0 > "${SMBCPLD_SYSFS_DIR}/pca9548_2_rst"
    sleep 1
    echo 1 > "${SMBCPLD_SYSFS_DIR}/pca9548_2_rst"
}

# Reset a specific PIM Mux.
reset_pim_mux() {
    pim="$1"

    echo "reset $pim i2c mux.."
    echo 0 > "${SMBCPLD_SYSFS_DIR}/${pim}_i2c_mux_rst_l"
    sleep 1
    echo 1 > "${SMBCPLD_SYSFS_DIR}/${pim}_i2c_mux_rst_l"
}

# Reset all the 8 PIM Muxes.
reset_all_pim_muxes() {
    smbcpld_bus=12
    smbcpld_addr="0x3e"
    smbcpld_reg="0x15"

    # Use i2cset to clear/set all the 8 bits, which is faster than
    # toggling "pim#_i2c_mux_rst_l" one by one.
    echo "reset 8 i2c muxes on PIMs.."
    i2cset -y -f "$smbcpld_bus" "$smbcpld_addr" "$smbcpld_reg" 0
    sleep 1
    i2cset -y -f "$smbcpld_bus" "$smbcpld_addr" "$smbcpld_reg" 0xff
}

if [ "$#" -ne 1 ]; then
    echo "Error: invalid command line argument!"
    usage
    exit 1
fi

if [ ! -e "$SMBCPLD_SYSFS_DIR" ]; then
    echo "Error: unable to locate smbcpld ($SMBCPLD_SYSFS_DIR)!"
    exit 1
fi

if [ "$1" = "all" ]; then
    reset_all_pim_muxes
    reset_smb_mux
elif [ "$1" = "smb" ]; then
    reset_smb_mux
elif [[ "$1" =~ pim[1-8] ]]; then
    reset_pim_mux "$1"
else
    echo "Error: invalid command line argument!"
    usage
    exit 1
fi
