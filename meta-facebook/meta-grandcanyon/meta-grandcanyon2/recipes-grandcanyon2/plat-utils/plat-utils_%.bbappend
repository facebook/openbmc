# Copyright 2020-present Facebook. All Rights Reserved.
#
#
# GrandCanyon 2.0 Project
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://ast-functions \
    file://power-on.sh \
    "

CFLAGS:prepend = " -DCONFIG_GRANDCANYON2 "