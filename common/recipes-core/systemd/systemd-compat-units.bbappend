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

LICENSE="GPL-2.0-only"
LIC_FILES_CHKSUM="file://hostname.service;beginline=3;endline=16;md5=0b1ee7d6f844d472fa306b2fee2167e0"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit systemd

SRC_URI += " \
        file://hostname.service \
        file://hostname.sh \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir} ${D}/usr/local/bin
    install -m 0644 hostname.service ${D}${systemd_system_unitdir}
    install -m 0755 hostname.sh ${D}/usr/local/bin/hostname.sh
    rm -f ${D}${sysconfdir}/init.d/hostname.sh
}

FILES:${PN} = "/usr/local/bin ${systemd_system_unitdir}"
SYSTEMD_SERVICE:${PN} = "hostname.service"
RDEPENDS:${PN}:append = " bash"
