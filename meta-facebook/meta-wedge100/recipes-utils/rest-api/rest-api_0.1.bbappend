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

inherit systemd

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://rest_i2cflush.py \
            file://rest_modbus.py \
            file://board_endpoint.py \
            file://boardroutes.py \
            file://board_setup_routes.py \
            file://rest_fw_ver.py \
            file://fake_kv.py \
            file://test_rest_fw_ver.py \
            file://restapi.service \
            file://rest_fw_ver.py \
            file://rest_presence.py \
           "

binfiles1 += "rest_i2cflush.py \
             rest_modbus.py \
             rest_fw_ver.py \
             rest_presence.py \
             "
binfiles += "board_endpoint.py \
             boardroutes.py \
             board_setup_routes.py \
            "
