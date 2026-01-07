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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../openbmc-utils/files/fw_upgrade/yamp-ufw-jsons:"

LOCAL_URI += " \
    file://rest_piminfo.py \
    file://rest_pimserial.py \
    file://rest_pimstatus.py \
    file://rest_pim_present.py \
    file://rest_sensors.py \
    file://rest_seutil.py \
    file://rest_peutil.py \
    file://rest_scdinfo.py \
    file://rest_fw_ver.py \
    file://rest_server.py \
    file://yamp_ufw_manifest.json \
"

binfiles1 += " \
    rest_piminfo.py \
    rest_pimserial.py \
    rest_pimstatus.py \
    rest_pim_present.py \
    rest_sensors.py \
    rest_seutil.py \
    rest_peutil.py \
    rest_scdinfo.py \
    rest_fw_ver.py \
    rest_server.py \
"

do_install:append() {
    install -m 644 ${UNPACKDIR}/yamp_ufw_manifest.json ${D}${sysconfdir}/ufw_manifest.json
}
