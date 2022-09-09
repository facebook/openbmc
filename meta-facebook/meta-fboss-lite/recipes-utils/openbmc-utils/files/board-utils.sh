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

#
# OVERRIDE_ME: this file needs to be overridden by platform!
#

userver_power_is_on() {
    echo "userver_power_is_on() not implemented. Assuming uServer is off!"
    return 1
}

userver_power_on() {
    echo "userver_power_on() not implemented. Exiting.."
    return 1
}

userver_power_off() {
    echo "userver_power_off() not implemented. Exiting.."
    return 1
}

userver_reset() {
    echo "userver_reset() not implemented. Exiting.."
    return 1
}

chassis_power_cycle() {
    echo "chassis_power_cycle() not implemented. Exiting.."
    return 1
}

bmc_mac_addr() {
    echo ""
}

userver_mac_addr() {
    echo ""
}
