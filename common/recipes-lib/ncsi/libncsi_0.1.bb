# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "NC-SI Library"
DESCRIPTION = "library to provide NC-SI functionality"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ncsi.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://ncsi.c \
    file://ncsi.h \
    file://aen.c \
    file://aen.h \
    file://ncsi-brcm.c \
    file://ncsi-brcm.h \
    file://ncsi-nvidia.c \
    file://ncsi-nvidia.h \
    "

inherit meson pkgconfig

DEPENDS += "libkv"
