# Copyright 2015-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:append := "${THISDIR}/files:"
LOCAL_URI += "file://healthd-config.json"

LOCAL_URI += " \
        file://healthd_plat.c \
        file://plat/meson.build \
        "

DEPENDS += "libbic"
