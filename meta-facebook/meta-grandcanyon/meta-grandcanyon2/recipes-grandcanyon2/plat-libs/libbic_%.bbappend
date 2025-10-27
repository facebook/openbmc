# Copyright 2020-present Facebook. All Rights Reserved.
#
#
# GrandCanyon 2.0 Project
FILESEXTRAPATHS:prepend := "${THISDIR}/files/bic:"

## Because bb file no UNPACKDIR define, using subdir to overwrite it
SRC_URI += " \
    file://bic_bios_fwupdate.c;subdir=bic \
    file://bic_bios_fwupdate.h;subdir=bic \
    file://bic_bios_usb_fwupdate.c;subdir=bic \
    file://bic_fwupdate.c;subdir=bic \
    file://bic_fwupdate.h;subdir=bic \
    file://bic_ipmi.c;subdir=bic \
    file://bic_ipmi.h;subdir=bic \
    file://bic_power.c;subdir=bic \
    file://bic_power.h;subdir=bic \
    file://bic_vr_fwupdate.c;subdir=bic \
    file://bic_vr_fwupdate.h;subdir=bic \
    file://bic_xfer.c;subdir=bic \
    file://bic_xfer.h;subdir=bic \
    file://bic.h;subdir=bic \
    "



