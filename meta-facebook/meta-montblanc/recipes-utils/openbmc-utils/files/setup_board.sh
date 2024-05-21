#!/bin/bash
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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

#
# XXX this function is used to configure 88E1512 PHY between BMC and OOB
#     switch on EVT units. The PHY is removed in DVT and newer revisions,
#     and we can drop the code once all the EVT units are retired.
#
fixup_phy_onlyevt() {
    MDIO_CH=2
    PHY_ADDR=0x0
    PAGE_REG=0x16

    # disable RGMII Delay
    # page 0x02 register 0x15
    #      bit [5] - 0 Recive clock transition when data transtions
    #          [4] - 0 Transmit clock not internal delayed
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w $PAGE_REG -d 0x2
    value=$(mdio-util -m $MDIO_CH -p $PHY_ADDR -r 0x15 | cut -d' ' -f7-)
    value=$(( value & 0xffcf ))
    value=$(printf 0x%04x "$value")
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w 0x15 -d "$value"

    # set mode RGMII to 1000Base-X
    # page 0x12 regiser 0x14
    #      bit [2:0] - 0x2 RGMII (System mode) to 1000Base-X
    #      bit [15]  - Mode software reset, 1 = PHY reset,
    #                  cleared to 0 automatically
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w $PAGE_REG -d 0x12
    value=$(mdio-util -m $MDIO_CH -p $PHY_ADDR -r 0x14 | cut -d' ' -f7-)
    value=$(( value & 0x7ff8 ))
    value=$(( value | 0x8002 ))
    value=$(printf 0x%04x "$value")
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w 0x14 -d "$value"

    # set page back to 0
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w $PAGE_REG -d 0x0

    echo -e "\nsetting PHY CHIP 88E1512 mode."
}


fixup_phy_onlyevt
# set LPC signal strength pin to 0 (weakest)
setup_LPC_signal_strength 0
# set MAC4 TX4 clock delay to 8
ast2600_setup_RGMII34_clock_delay MAC4_TX=8
