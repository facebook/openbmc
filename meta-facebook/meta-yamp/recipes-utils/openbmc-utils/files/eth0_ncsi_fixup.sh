#!/bin/bash
#
# Copyright 2024-present Facebook. All Rights Reserved.
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
# This script monitors eth0 for NCSI connectivity issues and performs
# recovery by cycling the interface if no global IPv6 address is found.
# Runs every 5 minutes.

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

# Wait for network to be configured on first boot
sleep 180

while true; do
    # Check if eth0 has a global IPv6 address
    if ! ip -6 addr show eth0 | grep -q "scope global"; then
        logger -t "eth0_ncsi_fixup" -p daemon.crit "eth0 has no global IPv6, performing recovery"
        ifdown eth0
        sleep 2
        ifup eth0
        logger -t "eth0_ncsi_fixup" -p daemon.info "eth0 recovery completed"
    fi

    # Check every 5 minutes
    sleep 300
done
