# Copyright 2023-present Meta. All Rights Reserved.
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

SUMMARY = "Subscribes to events of downstream devices supporting Redfish"
DESCRIPTION = "Redfish event service subscriber."
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

LOCAL_URI = " \
    file://redfish-events.py \
    file://redfish-events.cfg \
    file://redfishevents.service \
    file://run-redfish-events.sh \
    file://setup-redfish-events.sh \
    file://setup.py \
    "

inherit setuptools3
inherit systemd
DEPENDS += "update-rc.d-native"
RDEPENDS:${PN} += "python3-syslog python3-ply python3-aiohttp"

install_sysv() {
    install -d ${D}${sysconfdir}/sv
    install -d ${D}${sysconfdir}/sv/redfishevents
    install -m 755 ${S}/run-redfish-events.sh ${D}${sysconfdir}/sv/redfishevents/run
    install -d ${D}${sysconfdir}/init.d
    install -d ${D}${sysconfdir}/rcS.d
    install -m 755 ${S}/setup-redfish-events.sh ${D}${sysconfdir}/init.d/setup-redfish-events.sh
    update-rc.d -r ${D} setup-redfish-events.sh start 95 2 3 4 5  .
}

install_systemd() {
    install -d ${D}${systemd_system_unitdir}
    install -m 644 redfishevents.service ${D}${systemd_system_unitdir}
}

do_install:append() {
    bin="${D}/usr/bin"
    install -d $bin
    install -d ${D}${sysconfdir}
    install -m 644 ${S}/redfish-events.cfg ${D}${sysconfdir}/redfish-events.cfg
    install -m 755 ${S}/redfish-events.py ${bin}/redfish-events.py

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install_systemd
    else
        install_sysv
    fi
}

FILES:${PN} += "${prefix}/bin ${sysconfdir} "

SYSTEMD_SERVICE:${PN} = "redfishevents.service"
