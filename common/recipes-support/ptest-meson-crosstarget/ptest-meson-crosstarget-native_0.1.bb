SUMMARY = "Script to allow meson data to be used on targets"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"

SECTION = "libs"

inherit python3native
inherit native

SRC_URI = "file://ptest-meson-crosstarget"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/ptest-meson-crosstarget ${D}${bindir}
}
