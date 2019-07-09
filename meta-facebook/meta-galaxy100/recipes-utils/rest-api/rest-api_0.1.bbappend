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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://rest-api-1/eeprom_utils.py \
    file://rest-api-1/rest_fruid_scm.py \
    file://rest-api-1/rest_seutil.py \
    file://rest-api-1/rest_firmware.py \
    file://rest-api-1/rest_chassis_eeprom.py \
    file://rest-api-1/rest_i2cflush.py \
    file://board_endpoint.py \
    file://boardroutes.py \
    file://board_setup_routes.py \
    "

binfiles1 += " \
    eeprom_utils.py \
    rest_fruid_scm.py \
    rest_seutil.py \
    rest_firmware.py \
    rest_chassis_eeprom.py \
    rest_i2cflush.py \
    "

binfiles += " \
    board_endpoint.py \
    board_setup_routes.py \
    boardroutes.py \
    "
