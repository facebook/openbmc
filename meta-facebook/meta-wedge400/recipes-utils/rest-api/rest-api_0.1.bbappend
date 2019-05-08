# Copyright 2019-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://rest-api-1/rest_firmware_info.py \
    file://rest-api-1/rest_presence.py \
    file://rest-api-1/rest_seutil.py \
    file://rest-api-1/rest_sensors.py \
    file://rest-api-1/rest_usb2i2c_reset.py \
"

binfiles1 += " \
    rest_firmware_info.py \
    rest_presence.py \
    rest_sensors.py \
    rest_seutil.py \
    rest_usb2i2c_reset.py \
"
