# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing support for aggregated sensors"
DESCRIPTION = "Library providing support for aggregated sensors"
SECTION = "libs"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://aggregate-sensor.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson ptest-meson

SRC_URI = "file://meson.build \
           file://aggregate-sensor.h \
           file://aggregate-sensor.c \
           file://math_expression.h \
           file://math_expression.c \
           file://aggregate-sensor-internal.h \
           file://aggregate-sensor-json.c \
           file://aggregate-sensor-conf.json \
          "

SRC_URI += "file://test/aggregate-sensor-test.c \
           file://test/test_null.json \
           file://test/test_lexp.json \
           file://test/test_lexp_sexp.json \
           file://test/test_clexp.json \
          "

S = "${WORKDIR}"
export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

test_conf = "test_null.json test_lexp.json test_lexp_sexp.json test_clexp.json"
do_install_ptest_append() {
  for f in ${test_conf}; do
    install -m 755 ${WORKDIR}/test/$f ${D}${libdir}/libaggregate-sensor/ptest/$f
  done
}

do_install_append() {
  install -D -m 644 ${WORKDIR}/aggregate-sensor-conf.json ${D}${sysconfdir}/aggregate-sensor-conf.json
}

LDFLAGS= "-lm -lpal"

DEPENDS =+ "libpal libsdr jansson libkv cmock"
RDEPENDS_${PN} =+ "libpal libsdr libkv jansson "
FILES_${PN}-ptest += "${libdir}/libaggregate-sensors/ptest"
FILES_${PN} += "${sysconfdir}/aggregate-sensor-conf.json"
