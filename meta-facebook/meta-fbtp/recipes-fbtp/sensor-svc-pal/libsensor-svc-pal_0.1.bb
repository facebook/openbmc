# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Sensor Service Platform Abstraction Library"
DESCRIPTION = "Stripped pal library for sensor-svc"
SECTION = "libs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sensorsvcpal.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://sensorsvcpal.c \
    file://sensorsvcpal.h \
    file://CMakeLists.txt \
    "

DEPENDS += "libkv plat-utils libipmi libipmb obmc-pal libme libvr libobmc-i2c"


RDEPENDS:${PN} += " libkv libme libipmb libvr libobmc-i2c"

inherit cmake
