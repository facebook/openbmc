FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "OpenBMC CXL Coredump Collection Tool"
DESCRIPTION = "OpenBMC CXL Coredump Collection Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

RDEPENDS:${PN} += "bash expect"

DEPENDS += " \
    cli11 \
"

inherit meson pkgconfig

S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://cxl-coredump-collection.cpp \
    file://cxl-coredump-collection.hpp \
    file://cxl-coredump-collection.expect \
    file://meson.build \
    "

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/cxl-coredump-collection.expect ${D}/${bindir}/cxl-coredump-collection-uart
}

