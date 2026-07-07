# Copyright 2022-present Facebook. All Rights Reserved.

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# me-functions.cpp now reads SPD in-process via libpldm AF-MCTP (was popen pldmtool)
DEPENDS:append = " libpldm"

LOCAL_URI += " \
        file://plat/meson.build \
        file://me-functions.cpp \
        file://dimm-util-plat.h \
        "
