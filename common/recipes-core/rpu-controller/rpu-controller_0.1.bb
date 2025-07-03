# Copyright 2025-present Facebook. All Rights Reserved.
SUMMARY = "RPU Controller"
DESCRIPTION = "Scripts to start, stop, restart the connected RPUs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit systemd

S="${WORKDIR}/sources"
UNPACKDIR="${S}"

LOCAL_URI = " \
    file://default-config.json \
    file://rpu-controller-command \
    file://rpu-ready \
"

RDEPENDS:${PN}:append = "bash rackmon"

#SYSTEMD_SERVICE:${PN} = "rpu.service" TODO in a later revision

do_install() {
    install -d ${D}${libexecdir}/${PN}
    install -m 0755 ${UNPACKDIR}/rpu-controller-command ${D}${libexecdir}/${PN}/rpu-controller-command
    install -m 0755 ${UNPACKDIR}/rpu-ready ${D}${libexecdir}/${PN}/rpu-ready

    install -d ${D}/var/lib/${PN}
    install ${UNPACKDIR}/default-config.json ${D}/var/lib/${PN}/default-config.json
}
