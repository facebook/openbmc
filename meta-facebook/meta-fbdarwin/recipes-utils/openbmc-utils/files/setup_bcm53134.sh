#!/bin/bash
#
# Copyright 2022-present Facebook. All Rights Reserved.
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

### BEGIN INIT INFO
# Provides:          setup_bcm53134.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Setup the speed on BCM53134 for eth0
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

control_page="0x0"
port_state_reg="0xe"
port_state_reg_val="0x8b"
imp_rgmi_control_reg="0x60"
imp_rgmi_control_reg_val="0x0"
ieee_vlan_control_page="0x34"
vlan_port2_reg="0x14"
vlan_port2_val="0xffa"

# Check BCM53134 registers
curr_speed=$(oob-mdio-util.sh read8 "$control_page" "$port_state_reg"|
             cut -d' ' -f2)
curr_clk_dly=$(oob-mdio-util.sh read8 "$control_page" "$imp_rgmi_control_reg"|
               cut -d' ' -f2)
if [ "$curr_speed" == "$port_state_reg_val" ] && \
   [ "$curr_clk_dly" == "$imp_rgmi_control_reg_val" ]
then
    # Register values are as expected
    exit 0
fi

echo "BCM53134 registers not as expected. Configuring now."
echo "Re-program the BCM53134 EEPROM to fix this permanently."

oob-mdio-util.sh write8 "$control_page" "$port_state_reg" "$port_state_reg_val"
oob-mdio-util.sh write8 "$control_page" "$imp_rgmi_control_reg" \
                        "$imp_rgmi_control_reg_val"
# Enable Port 2 which is used on some platforms
oob-mdio-util.sh write16 "$ieee_vlan_control_page" "$vlan_port2_reg" "$vlan_port2_val"
exit 0
