# Copyright 2018-present Facebook. All Rights Reserved.
SUMMARY = "Voltage Regulator Library"
DESCRIPTION = "Library for communication with the voltage regulator"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://vr.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://meson.build \
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
          "

DEPENDS += "libobmc-pmbus libkv libpal libobmc-i2c "
RDEPENDS_${PN} += "libobmc-pmbus libkv libpal libobmc-i2c "

S = "${WORKDIR}"

inherit meson
