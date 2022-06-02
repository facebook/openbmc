# Copyright 2022-present Facebook. All Rights Reserved.
SUMMARY = "Openbmc PECI sensors library"
DESCRIPTION = "Openbmc PECI sensors Library"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://meson.build;beginline=4;endline=16;md5=368bf438400f6ca8e2d1ea81b2495b95"

LOCAL_URI = " \
  file://meson.build \
  file://peci_sensors.h \
  file://peci_sensors.c \
  "

S = "${WORKDIR}"

DEPENDS += " libpeci"
RDEPENDS:${PN} += " libpeci"
LDFLAGS += " -lpeci"

inherit meson pkgconfig
