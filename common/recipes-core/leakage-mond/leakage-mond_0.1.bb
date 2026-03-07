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

inherit systemd meson

SUMMARY = "Leakage monitor Daemon"
DESCRIPTION = "Daemon to monitor Leakage and add event log"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://leakage-mond.c;beginline=4;endline=16;md5=c0c4d931761f4e275ee79609dae525b1"

LDFLAGS = "-llog -lmisc-utils"

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://meson.build \
    file://leakage-mond.c \
    file://leakage-mond.service \
    "

DEPENDS += " liblog libmisc-utils"
RDEPENDS:${PN} += " liblog bash libmisc-utils"

do_install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/leakage-mond.service ${D}${systemd_system_unitdir}
}

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d $dst
    install -d $bin
    install -m 0755 leakage-mond ${dst}/leakage-mond
    ln -snf ../fbpackages/${pkgdir}/leakage-mond ${bin}/leakage-mond

    # Note: This feature is only supported on systemd platforms.
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        do_install_systemd
    else
        bb.fatal "leakage-mond is currently only supported on systemD based systems. Not installing leakage-mond."
    fi
}

FILES:${PN} = "${prefix}/local/fbpackages/${pkgdir}/leakage-mond ${prefix}/local/bin ${sysconfdir} ${systemd_system_unitdir}"
SYSTEMD_SERVICE:${PN} = "leakage-mond.service"

pkgdir = "leakage-mond"
