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
inherit systemd

SUMMARY = "Rackmon Functionality"
DESCRIPTION = "Rackmon Functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://modbus.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS_append = " update-rc.d-native"

LDFLAGS += "-llog"
DEPENDS += "libgpio liblog"
RDEPENDS_${PN} = "libgpio liblog python3-core bash"

SRC_URI = "file://Makefile \
           file://modbuscmd.c \
           file://modbussim.c \
           file://modbus.c \
           file://modbus.h \
           file://gpiowatch.c \
           file://rackmond.c \
           file://rackmond.h \
           file://rackmonctl.c \
           file://rackmon_platform.c \
           file://rackmon_platform.h \
           file://setup-rackmond.sh \
           file://run-rackmond.sh \
           file://rackmon-config.py \
           file://rackmond.py \
           file://psu-update-delta.py \
           file://psu-update-bel.py \
           file://psu-update-artesyn.py \
           file://pyrmd.py \
           file://srec.py \
           file://hexfile.py \
           file://rackmond.service \
           file://configure-rackmond.service \
          "

S = "${WORKDIR}"

binfiles = "modbuscmd \
            modbussim \
            gpiowatch \
            rackmond \
            rackmonctl \
            psu-update-delta.py \
            psu-update-bel.py \
            psu-update-artesyn.py \
            hexfile.py \
            pyrmd.py \
            srec.py \
           "

#otherfiles = "README"

pkgdir = "rackmon"

install_systemd() {
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 rackmond.service ${D}${systemd_system_unitdir}
    install -m 0644 configure-rackmond.service ${D}${systemd_system_unitdir}
}

install_sysv() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/rackmond
    install -m 755 run-rackmond.sh ${D}${sysconfdir}/sv/rackmond/run
    install -m 755 setup-rackmond.sh ${D}${sysconfdir}/init.d/rackmond
    update-rc.d -r ${D} rackmond start 95 2 3 4 5  .
}

do_install() {
    dst="${D}/usr/local/fbpackages/${pkgdir}"
    bin="${D}/usr/local/bin"
    install -d ${D}${sysconfdir}
    install -d $dst
    install -d $bin
    for f in ${binfiles}; do
        install -m 755 $f ${dst}/$f
        ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
    done
    ln -snf ../fbpackages/${pkgdir}/rackmonctl ${bin}/rackmondata
    ln -snf ../fbpackages/${pkgdir}/rackmonctl ${bin}/rackmonstatus
    ln -snf ../fbpackages/${pkgdir}/rackmonctl ${bin}/rackmonscan
    install -m 755 rackmon-config.py ${D}${sysconfdir}/rackmon-config.py
    install -m 755 rackmond.py ${D}${sysconfdir}/rackmond.py

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/rackmon ${prefix}/local/bin ${sysconfdir} "

FILES_${PN} += "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '${systemd_system_unitdir}', '', d)}"

# Inhibit complaints about .debug directories for the rackmon binaries:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"

SYSTEMD_SERVICE_${PN} = "rackmond.service configure-rackmond.service"
