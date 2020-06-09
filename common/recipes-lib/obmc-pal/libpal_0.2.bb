# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "OpenBMC PAL"
DESCRIPTION = "Platform Abstraction Layer for OpenBMC"

SECTION = "libs"
PR = "r1"

LICENSE = "LGPLv2"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/LGPL-2.1;md5=1a6d268fd218675ffea8be556788b780"

SRC_URI = "\
    file://meson.build \
    file://meson_options.txt \
    file://obmc-pal.c \
    file://obmc-pal.h \
    file://obmc_pal_sensors.c \
    file://obmc_pal_sensors.h \
    file://pal.c \
    file://pal.h \
    file://pal_sensors.h \
    "

DEPENDS += " \
    libipmb \
    libipmi \
    libkv \
    obmc-pal \
    "

EXTRA_OEMESON_append = "-Dmachine=\"${MACHINE}\""

S = "${WORKDIR}"

inherit meson
