# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Voltage Regulator Library"
DESCRIPTION = "Library for communication with the voltage regulator"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://vr.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://plat/meson.build \
    file://mpq8645p.c \
    file://mpq8645p.h \
    file://platform.c \
    file://pxe1110c.c \
    file://pxe1110c.h \
    file://tps53688.c \
    file://tps53688.h \
    file://vr.c \
    file://vr.h \
    file://xdpe12284c.c \
    file://xdpe12284c.h \
    file://xdpe152xx.c \
    file://xdpe152xx.h \
    file://raa_gen3.c \
    file://raa_gen3.h \
    "

DEPENDS += "libobmc-pmbus libkv libpal libobmc-i2c "
RDEPENDS:${PN} += "libobmc-pmbus libkv libpal libobmc-i2c "


inherit meson pkgconfig
