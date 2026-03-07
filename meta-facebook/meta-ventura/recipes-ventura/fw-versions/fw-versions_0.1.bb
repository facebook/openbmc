SUMMARY = "Firmware version retriever"
DESCRIPTION = "Scripts to retrieve firmware versions and update settingsd"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd

RDEPENDS:${PN}:append = " \
    bash \
    cpld-fw-handler \
    fw-util \
    "

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://fw-versions.service \
    file://rmc-all-versions \
    "

SYSTEMD_SERVICE:${PN} = " \
    fw-versions.service \
    "

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${libexecdir}/${BPN}
    install -m 0755 ${S}/rmc-all-versions ${D}${libexecdir}/${BPN}
    install -m 0644 ${S}/fw-versions.service ${D}${systemd_system_unitdir}
}
