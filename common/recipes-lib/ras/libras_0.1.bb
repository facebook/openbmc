# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "RAS Library"
DESCRIPTION = "RAS library. Reliability, availability and serviceability (RAS), is a computer hardware engineering term involving reliability engineering, high availability, and serviceability design"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://obmc-ras.c;beginline=1;endline=1;md5=b9fee9b0fbd6c5d36637dcaca2e0752e"

LOCAL_URI = " \
    file://obmc-ras.h \
    file://obmc-ras.c \
    file://ras.h \
    file://ras.c \
    file://meson.build \
    "
    
DEPENDS += " libgpio-ctrl "
RDEPENDS:${PN} += " libgpio-ctrl "

inherit meson
