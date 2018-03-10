#!/usr/bin/env python3
#
# Copyright 2004-present Facebook. All rights reserved.
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
# This script combines multiple switch configuration into one python script.
# All such configuration can be done directly through bcm5396_util.py.
# But, it turns out it took several seconds to just start the python script.
# Involking the script 16 times (2 to create vlan, 13 to set port vlan default,
# and 1 to enable vlan function) contributes 1 minute delay.

from bcm5396 import Bcm5396

MDC_GPIO = 6
MDIO_GPIO = 7

INTERNAL_VLAN = 4088
DEFAULT_VLAN=4090

INTERNAL_PORTS = [3, 10, 1, 11, 0, 8, 2, 9, 4, 12, 14, 13]
FRONT_PORT=5

if __name__ == '__main__':
    bcm = Bcm5396(Bcm5396.MDIO_ACCESS, mdc=MDC_GPIO, mdio=MDIO_GPIO)
    # create default VLAN including internal ports and front panel
    # port (un-tagged)
    bcm.add_vlan(DEFAULT_VLAN, INTERNAL_PORTS + [FRONT_PORT],
                  INTERNAL_PORTS + [FRONT_PORT], 0)
    # set ingress vlan for internal ports and front panel port to default vlan
    for port in INTERNAL_PORTS + [FRONT_PORT]:
        bcm.vlan_set_port_default(port, DEFAULT_VLAN, 0)
    # create internal vlan including internal ports only (tagged)
    bcm.add_vlan(INTERNAL_VLAN, [], INTERNAL_PORTS, 0)
    # enable vlan
    bcm.vlan_ctrl(True)
