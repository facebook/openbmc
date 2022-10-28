# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "MCU Library"
DESCRIPTION = "MCU Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://mcu.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build\
    file://mcu.c \
    file://mcu.h \
    "

inherit meson pkgconfig

DEPENDS = " libipmb libipmi libobmc-i2c libpal libkv"
