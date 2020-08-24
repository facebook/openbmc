# Copyright 2015-present Facebook. All Rights Reserved.
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
SUMMARY = "Configure the BMC"
DESCRIPTION = "The script communicates with host and configures BMC."
SECTION = "base"
PR = "r3"
LICENSE = "GPLv2"

LIC_FILES_CHKSUM = "file://spatula_wrapper.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"
inherit systemd

DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://setup-spatula.sh \
           file://spatula.conf \
           file://spatula_wrapper.py \
           file://spatula.service \
          "

S = "${WORKDIR}"

binfiles = "spatula_wrapper.py"

systemd_install() {
    install -m 644 spatula.service ${D}${systemd_system_unitdir}
    sed -i "s:@sysconfdir@:${sysconfdir}/default:" ${D}${systemd_system_unitdir}/spatula.service
}

sysv_install() {
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 setup-spatula.sh ${D}${sysconfdir}/init.d/setup-spatula.sh
    update-rc.d -r ${D} setup-spatula.sh start 95 2 3 4 5  .
}

do_install() {
    bin="${D}/usr/local/bin"
    install -d $bin
    install -d "${D}${systemd_system_unitdir}"
    install -d "${D}${sysconfdir}/default"
    install -m 644 spatula.conf ${D}${sysconfdir}/default/spatula.conf
    for f in ${binfiles}; do
        install -m 755 $f ${bin}/$f
    done

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',  d)}; then
        systemd_install
    else
        sysv_install
    fi
}

FILES_${PN} = "${prefix}/local/bin ${sysconfdir} ${systemd_system_unitdir}"
SYSTEMD_SERVICE_${PN} = "spatula.service"
