SUMMARY = "OpenBMC BIOS Update Tool"
DESCRIPTION = "OpenBMC BIOS Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

inherit meson pkgconfig

LOCAL_URI = " \
    file://bios-update.cpp \
    file://bios-update.hpp \
    file://bios-usb-update.cpp \
    file://bios-usb-update.hpp \
    file://meson.build \
    "

DEPENDS += " \
    cli11 \
    libusb1 \
    sdbusplus \
    "
