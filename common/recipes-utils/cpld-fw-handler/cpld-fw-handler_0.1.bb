SUMMARY = "OpenBMC CPLD Firmware Handler"
DESCRIPTION = "OpenBMC CPLD firmware upgrade and version management"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

inherit meson pkgconfig

LOCAL_URI = " \
    file://cpld-fw-handler.cpp \
    file://cpld-fw-handler.hpp \
    file://cpld-fw-main.cpp\
    file://cpld-lattice.cpp \
    file://cpld-lattice.hpp \
    file://meson.build \
    "

DEPENDS += "cli11"
