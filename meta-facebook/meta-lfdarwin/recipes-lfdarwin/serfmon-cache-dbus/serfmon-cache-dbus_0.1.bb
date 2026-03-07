# Copyright 2022-present Facebook. All Rights Reserved.
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

SUMMARY = "Serial Number Provider"
DESCRIPTION = "Provide a custom DBUS interface for the x86 Serial Number"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://server.cpp;beginline=3;endline=14;md5=d5c79db633b2cf2b9915f42487fbf620"

inherit meson pkgconfig systemd

S = "${UNPACKDIR}"
SRC_URI = " \
    file://server.cpp \
    file://meson.build \
    file://gen/ \
    file://yaml/ \
    file://serfmon-cache-dbus.service \
"

DEPENDS += " \
    fmt \
    sdbusplus \
    ${PYTHON_PN}-sdbus++-native \
"

RDEPENDS:${PN} += " \
    fmt \
    "

do_install() {
    install -d ${D}/usr/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${B}/server ${D}/usr/bin/serfmon-cache-dbus
    install -m 0644 ${UNPACKDIR}/serfmon-cache-dbus.service ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${prefix}/bin"
SYSTEMD_SERVICE:${PN} = "serfmon-cache-dbus.service"
