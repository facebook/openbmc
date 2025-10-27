# Copyright 2020-present Facebook. All Rights Reserved.
#
#
# GrandCanyon 2.0 Project
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
## Because bb file have UNPACKDIR define, don't need subdir
LOCAL_URI += " \
    file://usb-dbg-conf.c \
    "

