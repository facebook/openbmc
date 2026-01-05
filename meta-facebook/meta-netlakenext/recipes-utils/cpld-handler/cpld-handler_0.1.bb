SUMMARY = "OpenBMC CPLD Handler"
DESCRIPTION = "OpenBMC CPLD firmware upgrade and version management"

# The cpld-handler was moved from common/recipes-utils/cpld-handler/files/. Due to the C++ version being changed from C++20 to C++17 and the Meson version being downgraded from 0.57 to 0.53, this modification was made.
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

inherit meson pkgconfig

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://cpld-handler.cpp \
    file://cpld-handler.hpp \
    file://cpld-main.cpp\
    file://cpld-lattice.cpp \
    file://cpld-lattice.hpp \
    file://meson.build \
    file://meson_options.txt \
    "

EXTRA_OEMESON:append:npcm8xx = " -Dupdate-ebr-init=enabled"
DEPENDS += "cli11"
