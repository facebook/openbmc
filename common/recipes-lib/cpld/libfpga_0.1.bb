# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "CPLD Library"
DESCRIPTION = "Library for communicating with CPLD"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://cpld.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://cpld.c \
           file://cpld.h \
           file://lattice.c \
           file://lattice.h \
           file://altera.c \
           file://altera.h \
           file://meson.build \
          "

DEPENDS = "libast-jtag libpal"
RDEPENDS_${PN} = "libast-jtag libpal"

S = "${WORKDIR}"

inherit meson
