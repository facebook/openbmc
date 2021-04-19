# Copyright 2018-present Facebook. All Rights Reserved.
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
    file://0001-common_setup_routes-rest_fruid_hdl-override.patch \
    file://rest_firmware_info_pim.py \
    file://rest_firmware_info_scm.py \
    file://rest_fruid_scm.py \
    file://rest_peutil.py \
    file://rest_pim_details.py \
    file://rest_piminfo.py \
    file://rest_pimserial.py \
    file://rest_pimstatus.py \
    file://rest_pim_present.py \
    file://rest_seutil.py \
    file://rest_usb2i2c_reset.py \
    file://rest_fw_ver.py \
    file://rest_system_led_info.py \
    file://test_rest_pim_details.py \
"

binfiles1 += " \
    rest_firmware_info_pim.py \
    rest_firmware_info_scm.py \
    rest_fruid_scm.py \
    rest_peutil.py \
    rest_pim_details.py \
    rest_piminfo.py \
    rest_pimserial.py \
    rest_pimstatus.py \
    rest_pim_present.py \
    rest_seutil.py \
    rest_usb2i2c_reset.py \
    rest_fw_ver.py \
    rest_system_led_info.py \
"
