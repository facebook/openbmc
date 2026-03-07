SUMMARY = "OpenBMC BIOS Update Tool"
DESCRIPTION = "OpenBMC BIOS Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig
S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://pldmutils/package_parser.cpp \
    file://pldmutils/package_parser.hpp \
    file://pldmutils/types.hpp \
    file://pldmutils/utils.hpp \
    file://pldmutils/utils.cpp \
    file://bios-update.cpp \
    file://bios-update.hpp \
    file://bios-usb-update.cpp \
    file://bios-usb-update.hpp \
    file://meson_options.txt \
    file://meson.build \
    "

EXTRA_OEMESON:append = " -Dplatform-name=Yosemite4 -Diana-number=40981 -Dvendor-name=meta -Dboard-name=SentinelDome"
DEPENDS += " \
    cli11 \
    libusb1 \
    sdbusplus \
    libpldm \
    phosphor-logging \
    "
