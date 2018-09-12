# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing support for aggregated sensors"
DESCRIPTION = "Library providing support for aggregated sensors"
SECTION = "libs"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://aggregate-sensor.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://CMakeLists.txt \
           file://aggregate-sensor.h \
           file://aggregate-sensor.c \
           file://math_expression.h \
           file://math_expression.c \
           file://aggregate-sensor-internal.h \
           file://aggregate-sensor-json.c \
           file://aggregate-sensor-conf.json \
          "

S = "${WORKDIR}"
export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

DEPENDS =+ "libpal libsdr jansson libkv "
RDEPENDS_${PN} =+ "libpal libsdr libkv jansson "

inherit cmake
