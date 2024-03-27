FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "OpenBMC PLDM Update package re-wrapping Tool"
DESCRIPTION = "OpenBMC re-wrapping PLDM update packages header"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

DEPENDS += " \
    libpldm \
    cli11 \
"

inherit meson pkgconfig

SRC_URI = " \
    file://meson.build \
    file://main.cpp \
    "
