# Copyright (c) Meta Platforms, Inc. and affiliates.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://rackmond-override.conf \
    file://check-rackmon-module.sh \
    file://setup-ftdi-device.sh \
    "

INTERFACE_CONFIG = "/usr/share/rackmon/interface/usb_ft4232.conf"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/rackmond.service.d
    install -m 0644 ${UNPACKDIR}/rackmond-override.conf ${D}${systemd_system_unitdir}/rackmond.service.d/rackmond-override.conf

    install -d ${D}/usr/local/bin
    install -m 0755 ${UNPACKDIR}/check-rackmon-module.sh ${D}/usr/local/bin
    install -m 0755 ${UNPACKDIR}/setup-ftdi-device.sh ${D}/usr/local/bin
}

FILES:${PN} += "${systemd_system_unitdir}/rackmond.service.d"
