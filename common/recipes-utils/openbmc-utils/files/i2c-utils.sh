# Copyright 2015-present Facebook. All Rights Reserved.
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

i2c_device_add() {
    local bus
    local addr
    local device
    bus=$"$1"
    addr="$2"
    device="$3"
    echo ${device} ${addr} > /sys/class/i2c-dev/i2c-${bus}/device/new_device
}

i2c_device_delete() {
    local bus
    local addr
    bus=$"$1"
    addr="$2"
    echo ${addr} > /sys/class/i2c-dev/i2c-${bus}/device/delete_device
}
