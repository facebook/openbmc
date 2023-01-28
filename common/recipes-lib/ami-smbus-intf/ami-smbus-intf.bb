SUMMARY = "AmiSmbusInterface Library"
DESCRIPTION = "AmiSmbusInterface Library"
SECTION = "libs"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

LOCAL_URI = " \
    file://AmiSmbusInterfaceSrcLib.h \
    file://AmiSmbusInterfaceSrcLib.c \
    file://meson.build \
"

inherit meson pkgconfig

DEPENDS += "libkv libobmc-i2c"

