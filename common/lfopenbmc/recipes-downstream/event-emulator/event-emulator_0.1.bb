SUMMARY = "Redfish Event Emulator"
DESCRIPTION = "Tool to emulate the Redfish events"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig
S = "${UNPACKDIR}"

LOCAL_URI = " \
    file://meson.build \
    file://event-emulator.cpp \
    file://devices \
    file://utils \
    "

DEPENDS += " \
    cli11 \
    nlohmann-json \
    phosphor-dbus-interfaces \
    phosphor-logging \
    sdbusplus \
    "
