#!/bin/sh
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

# The 88E6320 OOB switch is on the AST2600 MDIO0 controller (mdio-util bus 1)
# and is strapped for single-chip addressing: unlike the fboss-lite default
# (88E6321 at multi-chip address 0x12 on bus 2), its port registers are
# accessed directly, port N at PHY address 0x10 + N.
OOB_MDIO_BUS=1
OOB_PORT_BASE_ADDR=0x10

mode=$1
port=$2
reg_addr=$3

usage() {
    echo "OOB MDIO Utility to access Marvell 88E6320 OOB Switch"
    echo "Usage:"
    echo "$0 read <port> <reg_addr> "
    echo "$0 write <port> <reg_addr> <data>"
}

if [ $# -lt 3 ]; then
    usage
    exit 1;
fi

phy_addr=$(printf "0x%02X" $(( OOB_PORT_BASE_ADDR + port )))

if [ "$mode" = "write" ]; then
    if [ $# -ne 4 ]; then
        usage
        exit 1;
    fi
    data=$4
    mdio-util -m "$OOB_MDIO_BUS" -p "$phy_addr" -w "$reg_addr" -d "$data"
elif [ "$mode" = "read" ]; then
    if [ $# -ne 3 ]; then
        usage
        exit 1;
    fi
    mdio-util -m "$OOB_MDIO_BUS" -p "$phy_addr" -r "$reg_addr"
else
    usage
    exit 1;
fi
