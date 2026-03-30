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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
    file://rackmond-envsetup.sh \
    file://rackmond-envsetup.service \
    file://rackmond.conf \
    "

do_install:append() {
    bin="${D}/usr/libexec/rackmond"
    install -d ${bin}
    install -m 755 ${UNPACKDIR}/rackmond-envsetup.sh ${bin}/rackmond-envsetup
    install -m 0644 ${UNPACKDIR}/rackmond-envsetup.service ${D}${systemd_system_unitdir}/rackmond-envsetup.service

    fixups="${D}${sysconfdir}/systemd/system/rackmond.service.d"
    install -d ${fixups}
    install -m 0644 ${UNPACKDIR}/rackmond.conf ${fixups}/rackmond.conf
}

FILES:${PN} += "/usr/libexec/rackmond"
SYSTEMD_SERVICE:${PN} += "rackmond-envsetup.service"
