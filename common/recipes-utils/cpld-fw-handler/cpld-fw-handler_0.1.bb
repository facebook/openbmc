SUMMARY = "OpenBMC CPLD Firmware Handler"
DESCRIPTION = "OpenBMC CPLD firmware upgrade and version management"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig
S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://cpld-fw-handler.cpp \
    file://cpld-fw-handler.hpp \
    file://cpld-fw-main.cpp\
    file://cpld-lattice.cpp \
    file://cpld-lattice.hpp \
    file://xo5/xo5_bit_file_frame_parser.cpp \
    file://xo5/xo5_bit_file_frame_parser.hpp \
    file://xo5/xo5_sram_recover.cpp \
    file://xo5/xo5_sram_recover.hpp \
    file://meson.build \
    file://meson_options.txt \
    "

EXTRA_OEMESON:append:npcm8xx = " -Dupdate-ebr-init=enabled"
DEPENDS += " \
    cli11 \
    openssl \
    "
