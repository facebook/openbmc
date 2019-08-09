# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

BBCLASSEXTEND = "native"
CFLAGS += "-D__MACHINE__=${MACHINE}"

SRC_URI = "file://obmc-pal.h \
           file://obmc-pal.c \
           file://obmc-sensor.c \
           file://obmc-sensor.h \
           file://CMakeLists.txt \
          "
DEPENDS += " libkv libipmi libipmb"

inherit cmake

S = "${WORKDIR}"

RDEPENDS_${PN} += " libkv"
