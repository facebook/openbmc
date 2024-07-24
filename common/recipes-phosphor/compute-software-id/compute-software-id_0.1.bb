SUMMARY = "OpenBMC software ID compute tool"
DESCRIPTION = "OpenBMC software ID compute tool"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

inherit meson pkgconfig

LOCAL_URI = " \
    file://compute-software-id.cpp \
    file://meson.build \
    "

DEPENDS += " \
    cli11 \
    "
