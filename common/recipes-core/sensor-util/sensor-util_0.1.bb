# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Sensor Utility"
DESCRIPTION = "Util for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sensor-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit meson
inherit ptest-meson

SRC_URI = "file://meson.build \
           file://sensor-util.cpp \
          "

S = "${WORKDIR}"

pkgdir = "sensor-util"

DEPENDS =+ " libsdr libpal libaggregate-sensor jansson"
RDEPENDS:${PN} =+ "libsdr libpal libaggregate-sensor jansson"
FILES:${PN} = "${prefix}/local/bin/sensor-util"


