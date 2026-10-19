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
inherit systemd
DEPENDS += "systemd"

SUMMARY = "Update wedge host power status"
DESCRIPTION = "Update Host power status to D-bus"
SECTION = "base"
PR = "r1"
PV = "0.1"
LICENSE = "CLOSED"
#LIC_FILES_CHKSUM = ""

S = "${UNPACKDIR}"
FILESEXTRAPATHS:append := ":${THISDIR}/files"

LOCAL_URI = " \
    file://wedge_pwr_status_monitor.sh \
    file://wedge-power-status.service \
    "

do_install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/wedge-power-status.service ${D}${systemd_system_unitdir}
}

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d $dst
    install -d $bin
    install -m 0755 wedge_pwr_status_monitor.sh ${dst}/wedge_pwr_status_monitor.sh
    ln -snf ../fbpackages/${pkgdir}/wedge_pwr_status_monitor.sh ${bin}/wedge_pwr_status_monitor.sh

    # Note: This feature is only supported on systemd platforms.
    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        do_install_systemd
    else
        bb.fatal "wedge_pwr_status_monitor.sh is currently only supported on systemD based systems. Not installing wedge_pwr_status_monitor.sh."
    fi
}

FILES:${PN} = "${prefix}/local/fbpackages/${pkgdir}/wedge_pwr_status_monitor.sh ${prefix}/local/bin ${sysconfdir} ${systemd_system_unitdir}"
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "wedge-power-status.service"

pkgdir = "wedge-power-status"
