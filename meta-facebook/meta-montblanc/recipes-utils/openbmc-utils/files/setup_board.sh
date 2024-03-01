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
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w 0x15 -d $value

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
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w 0x14 -d $value

    # set page back to 0
    mdio-util -m $MDIO_CH -p $PHY_ADDR -w $PAGE_REG -d 0x0

    echo -e "\nsetting PHY CHIP 88E1512 mode."
}

# set LPC signal strngth to weakest
setup_LPC_signal_strength()
{
    SCU454=$(scu_addr 454)
    SCU458=$(scu_addr 458)

    # SCU454 : Multi-function Pin Control #15
    # 31:30 - LAD3/ESPID3 Driving Strength
    # 29:28 - LAD2/ESPID2 Driving Strength
    # 27:26 - LAD1/ESPID1 Driving Strength
    # 25:24 - LAD0/ESPID0 Driving Strength
    #          3: strongest    0: weakest
    # set strength value to 0
    value=$(devmem "$SCU454")
    value=$(( value & 0x00FFFFFF ))
    devmem "$SCU454" 32 $value
    
    # SCU458 : Multi-function Pin Control #16
    # 19:18 - LPCRSTN/ESPIRSTN Driving Strength
    # 17:16 - LACDIRQ/ESPIALERT Driving Strength
    # 15:14 - LPCFRAMEN/ESPICSN Driving Strength
    # 13:12 - LPCCLK/ESPICLK Driving Strength
    #          3: strongest    0: weakest
    # set strength value to 0
    value=$(devmem "$SCU458")
    value=$(( value & 0xFFF00FFF ))
    devmem "$SCU458" 32 $value
}

fixup_phy_onlyevt
setup_LPC_signal_strength
