# Copyright 2024-present Facebook. All Rights Reserved.

SUMMARY = "HMC Utility Tool"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

LOCAL_URI += " \
    file://hmc-util \
    "

RDEPENDS:${PN} += " bash"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/hmc-util ${D}${bindir}/hmc-util
}
