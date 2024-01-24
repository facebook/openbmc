SUMMARY = "Linux Foundation OpenBMC based mfg-tool utility"
DESCRIPTION = "A wrapper utility for manufacturing use cases"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://mfg-tool.cpp \
    file://cmd \
    file://utils \
    "

DEPENDS += " \
    cli11 \
    nlohmann-json \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    "
