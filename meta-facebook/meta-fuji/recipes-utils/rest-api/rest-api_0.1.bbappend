# Copyright 2020-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../openbmc-utils/files/fw_upgrade/fuji-ufw-jsons:"

LOCAL_URI += " \
    file://rest_feutil.py \
    file://rest_seutil.py \
    file://rest_peutil.py \
    file://rest_firmware_info.py \
    file://rest_presence.py \
    file://rest_sensors.py \
    file://rest_piminfo.py \
    file://rest_pimserial.py \
    file://rest_pim_present.py \
    file://rest_system_led_info.py \
    file://rest_smbinfo.py \
    file://rest_improve_aura_pll.py \
    file://fuji_ufw_manifest.json \
"

binfiles1 += " \
    rest_feutil.py \
    rest_seutil.py \
    rest_peutil.py \
    rest_firmware_info.py \
    rest_presence.py \
    rest_sensors.py \
    rest_piminfo.py \
    rest_pimserial.py \
    rest_pim_present.py \
    rest_system_led_info.py \
    rest_smbinfo.py \
    rest_improve_aura_pll.py \
"

do_install:append() {
    install -m 644 ${UNPACKDIR}/fuji_ufw_manifest.json ${D}${sysconfdir}/ufw_manifest.json
}
