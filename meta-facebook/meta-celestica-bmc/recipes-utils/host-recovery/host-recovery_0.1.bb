# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

SUMMARY = "OpenBMC Host-Recovery utilies"
SECTION = "base"
PR = "r0"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://service_mode.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://host-recovery.rules \
    file://service_mode.sh \
    file://spi-utils.sh \
    file://bios_update.sh \
    file://iob_update.sh \
    file://cpld_update.sh \
"

HOSTRECOVERY_FILES = " \
    service_mode.sh \
    spi-utils.sh \
    bios_update.sh \
    iob_update.sh \
    cpld_update.sh \
"

do_install() {
    pkgdir="/usr/local/fbpackages/host-recovery"
    dstdir="${D}/${pkgdir}"
    install -d ${dstdir}

    for f in ${HOSTRECOVERY_FILES}; do
        install -m 755 ${f} ${dstdir}/${f}
    done

    # install only service_mode.sh to /usr/local/bin
    localbindir="${D}/usr/local/bin"
    install -d ${localbindir}
    ln -s ${pkgdir}/service_mode.sh ${localbindir}/service_mode.sh

    # install udev rules
    udevdir="${D}/${sysconfdir}/udev/rules.d"
    install -d ${udevdir}
    install -m 644 host-recovery.rules ${udevdir}/90-host-recovery.rules
}

RDEPENDS:${PN} += " \
    bash \
    udev-rules \
    jbi \
    cpldupdate \
    libcpldupdate-dll-ast-jtag \
"

FILES:${PN} += "/usr/local/"
