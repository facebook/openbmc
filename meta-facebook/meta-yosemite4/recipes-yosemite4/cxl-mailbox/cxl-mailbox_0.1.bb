SUMMARY = "OpenBMC CXL MCTP CCI Tool"
DESCRIPTION = "OpenBMC CXL MCTP CCI Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig

LOCAL_URI = " \
    file://cxl-mailbox-info-collect \
    file://cxl-mailbox.cpp \
    file://cxl-mailbox.hpp \
    file://libcxl.cpp \
    file://libcxl.hpp \
    file://meson.build \
    "

DEPENDS += " \
    util-linux-libuuid \
    "

RDEPENDS:${PN} = "bash"

do_install:append() {
    install -d ${D}/${bindir}
    install -m 0755 ${UNPACKDIR}/cxl-mailbox-info-collect ${D}/${bindir}/
}
