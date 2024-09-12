FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "OpenBMC PLDM Update Tool"
DESCRIPTION = "OpenBMC PLDM Update Tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit meson pkgconfig

LOCAL_URI = " \
    file://pldm-update.hpp \
    file://pldm-update.cpp \
    file://options.cpp \
    file://meson.build \
    "

DEPENDS += " \
    cli11 \
    sdbusplus \
    libpldm \
    "

