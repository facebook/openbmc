#!/bin/bash
#
# Copyright 2014-present Facebook. All Rights Reserved.
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
# Provides:          eth0_mac_fixup.sh
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Fixup the MAC address for eth0 based on wedge EEPROM
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

#shellcheck disable=SC1091
source /usr/local/bin/openbmc-utils.sh

# get the MAC from EEPROM
if [ "$(LC_ALL=C type -t bmc_mac_addr)" = function ]; then
    mac=$(bmc_mac_addr)
else
    mac=$(weutil  | grep '^Local MAC' | cut -d' ' -f3)
fi

if [ -n "$mac" ]; then
    logger -p user.info "eth0_mac_fixup: Read MAC $mac from EEPROM"
fi

# get the MAC from u-boot environment
ethaddr=$(fw_printenv | grep "^ethaddr=" | cut -d'=' -f2)

if [ -z "$mac" ]; then
    logger -p user.err "eth0_mac_fixup: Failed to read MAC from EEPROM (empty or unavailable)"
    if [ -n "$ethaddr" ]; then
        # no MAC from EEPROM, use the one from u-boot environment
        logger -p user.info "eth0_mac_fixup: Falling back to U-Boot env MAC: $ethaddr"
        echo "No MAC address from EEPROM: use $ethaddr from uboot-env"
        mac="$ethaddr"
    else
        logger -p user.err "eth0_mac_fixup: Failed to read MAC from both EEPROM and U-Boot env"
        echo "Error: unable to read MAC address from EEPROM or uboot-env!"
        exit 1
    fi
fi

if [ "$ethaddr" != "$mac" ]; then
    # set the MAC from EEPROM back to u-boot environment so that
    # u-boot can use it
    fw_setenv "ethaddr" "$mac"
fi

# Compare current MAC with desired MAC
current_mac=$(cat /sys/class/net/eth0/address)
logger -p user.info "eth0_mac_fixup: Current eth0 MAC: $current_mac, desired MAC: $mac"

if [ "$(echo "$current_mac" | tr 'A-F' 'a-f')" = "$(echo "$mac" | tr 'A-F' 'a-f')" ]; then
    logger -p user.info "eth0_mac_fixup: eth0 MAC already matches desired MAC, skipping update"
    exit 0
fi

#ifconfig eth0 hw ether $macifconfig
echo "Update BMC eth0 MAC address to $mac"
if ip link set dev eth0 address "$mac"; then
    logger -p user.info "eth0_mac_fixup: Successfully set eth0 MAC to $mac"
else
    logger -p user.err "eth0_mac_fixup: Failed to set eth0 MAC address to $mac"
    exit 1
fi
exit 0
