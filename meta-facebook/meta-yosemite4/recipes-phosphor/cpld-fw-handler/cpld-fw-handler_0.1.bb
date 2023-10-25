SUMMARY = "OpenBMC CPLD Firmware Handler"
DESCRIPTION = "OpenBMC CPLD firmware upgrade and version management"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

S = "${WORKDIR}"
SRC_URI += " \
    file://cpld-fw-handler.cpp \
    file://cpld-fw-handler.hpp \
    file://cpld-lattice.cpp \
    file://cpld-lattice.hpp \
    file://cpld-fw-main.cpp\
    file://meson.build \
    "

SYSTEMD_PACKAGES = "${PN}"

DEPENDS += "cli11"

do_install () {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/build/cpld-fw-handler ${D}${sbindir}
}
