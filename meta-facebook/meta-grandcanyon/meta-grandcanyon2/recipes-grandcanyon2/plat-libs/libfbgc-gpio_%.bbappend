# Copyright 2020-present Facebook. All Rights Reserved.
#
#
# GrandCanyon 2.0 Project
FILESEXTRAPATHS:prepend := "${THISDIR}/files/fbgc_gpio:"
## Because bb file no UNPACKDIR define, using subdir to overwrite it
SRC_URI += " \
    file://fbgc_gpio.c;subdir=fbgc_gpio \
    file://fbgc_gpio.h;subdir=fbgc_gpio \
    "

