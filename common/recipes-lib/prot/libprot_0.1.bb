# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "Platform root of trust Library"
DESCRIPTION = "Library for communication with the Platfire firmware"
SECTION = "libs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-or-later;md5=fed54355545ffd980b814dab4a3b312c"

LOCAL_URI = " \
    file://ProtCommonInterface.hpp \
    file://ProtDevice.hpp \
    file://ProtDevice.cpp \
    file://ProtLog.hpp \
    file://ProtLog.cpp \
    file://meson.build \
"

inherit meson pkgconfig

DEPENDS += "ami-smbus-intf libobmc-i2c fmt"

