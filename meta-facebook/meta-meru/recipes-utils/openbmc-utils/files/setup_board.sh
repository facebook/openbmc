#!/bin/bash
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

SMB_EEPROM_SYSFS="/sys/bus/i2c/drivers/at24/9-0052/eeprom"
NETWORK_CONF_FILE="/etc/systemd/network/10-eth0.network"

if [ ! -e "$SMB_EEPROM_SYSFS" ]; then
    echo "No SMB eeprom found"
    exit 0
fi

product=$(weutil -e smb 2>&1 | awk -F': ' '/Product Name:/ {print $2}')

if [ "${product^^}" != "MERU800BFA" ]; then
    # No need to configure vlan 4092 for other products.
    exit 0
fi

echo "Configuring VLAN 4092"
vlan4088cfg="VLAN=eth0.4088"
vlan4092cfg="VLAN=eth0.4092"
sed -i "s/$vlan4088cfg/$vlan4088cfg\n$vlan4092cfg/" "$NETWORK_CONF_FILE"
cp /etc/meru-21-eth0.4092.network /etc/systemd/network/21-eth0.4092.network
cp /etc/meru-21-eth0.4092.netdev /etc/systemd/network/21-eth0.4092.netdev
