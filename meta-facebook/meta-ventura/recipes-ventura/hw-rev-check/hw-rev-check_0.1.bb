# Copyright 2024-present Facebook. All Rights Reserved.
SUMMARY = "Hardware revision checking"
DESCRIPTION = "Scripts to retrieve hardware revision and store in kv cache"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

S = "${UNPACKDIR}"

inherit systemd

SRC_URI += " \
    file://hw-rev-check \
    file://hw-rev-check.service \
"

RDEPENDS:${PN}:append = "bash mfg-tool"

SYSTEMD_SERVICE:${PN} = "hw-rev-check.service"

do_install() {
    install -d ${D}${libexecdir}/${BPN}
    install -m 0755 ${UNPACKDIR}/hw-rev-check ${D}${libexecdir}/${BPN}/hw-rev-check

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/hw-rev-check.service ${D}${systemd_system_unitdir}/hw-rev-check.service
}

FILES:${PN} += "${systemd_system_unitdir}"
