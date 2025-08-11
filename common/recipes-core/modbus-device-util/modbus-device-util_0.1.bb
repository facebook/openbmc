# Copyright 2025-present Facebook. All Rights Reserved.
SUMMARY = "RPU Controller"
DESCRIPTION = "Scripts to start, stop, restart the connected RPUs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit systemd

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://default-config.json \
    file://get-manufacturer \
    file://get-info \
    file://rpu-controller-command \
    file://rpu-ready \
    file://rpu@.service \
    file://start-rpu-service-and-get-logs \
"

RDEPENDS:${PN}:append = "bash rackmon"

SYSTEMD_SERVICE:${PN} += "rpu@.service"

do_install() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/rpu@.service ${D}${systemd_system_unitdir}

    install -d ${D}${libexecdir}/${PN}
    install -m 0755 ${UNPACKDIR}/get-manufacturer ${D}${libexecdir}/${PN}/get-manufacturer
    install -m 0755 ${UNPACKDIR}/get-info ${D}${libexecdir}/${PN}/get-info
    install -m 0755 ${UNPACKDIR}/start-rpu-service-and-get-logs ${D}${libexecdir}/${PN}/start-rpu-service-and-get-logs
    install -m 0755 ${UNPACKDIR}/rpu-controller-command ${D}${libexecdir}/${PN}/rpu-controller-command
    install -m 0755 ${UNPACKDIR}/rpu-ready ${D}${libexecdir}/${PN}/rpu-ready

    install -d ${D}/var/lib/${PN}
    install ${UNPACKDIR}/default-config.json ${D}/var/lib/${PN}/default-config.json
}
