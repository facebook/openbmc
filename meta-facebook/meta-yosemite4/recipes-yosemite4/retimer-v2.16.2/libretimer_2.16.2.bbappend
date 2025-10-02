# Copyright 2022-present Facebook. All Rights Reserved.

DEFAULT_PREFERENCE = "1"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += " \
        file://plat/meson.build \
        file://plat.c      \
        file://plat.h      \
        "

DEPENDS += " \
    libbic \
    "

RDEPENDS:${PN} += " \
    libbic \
    "