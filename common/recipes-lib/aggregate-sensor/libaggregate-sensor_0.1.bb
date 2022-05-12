# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing support for aggregated sensors"
DESCRIPTION = "Library providing support for aggregated sensors"
SECTION = "libs"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://aggregate-sensor.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig ptest-meson

LOCAL_URI = " \
    file://meson.build \
    file://aggregate-sensor.h \
    file://aggregate-sensor.c \
    file://math_expression.h \
    file://math_expression.c \
    file://aggregate-sensor-internal.h \
    file://aggregate-sensor-json.c \
    file://aggregate-sensor-conf.json \
    file://aggregate_sensor.py \
    file://test/aggregate-sensor-test.c \
    file://test/test_null.json \
    file://test/test_lexp.json \
    file://test/test_lexp_sexp.json \
    file://test/test_clexp.json \
    "

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

test_conf = "test_null.json test_lexp.json test_lexp_sexp.json test_clexp.json"
do_install_ptest:append() {
  for f in ${test_conf}; do
    install -m 755 ${S}/test/$f ${D}${libdir}/libaggregate-sensor/ptest/$f
  done
}

inherit python3-dir

do_install:append() {
  install -D -m 644 ${S}/aggregate-sensor-conf.json ${D}${sysconfdir}/aggregate-sensor-conf.json
  install -d ${D}${PYTHON_SITEPACKAGES_DIR}
  install -m 644 ${S}/aggregate_sensor.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}

LDFLAGS= "-lm -lpal"

DEPENDS =+ "libpal libsdr jansson libkv cmock"
RDEPENDS:${PN} =+ "libpal libsdr libkv jansson "
FILES:${PN}-ptest += "${libdir}/libaggregate-sensors/ptest"
FILES:${PN} += "${sysconfdir}/aggregate-sensor-conf.json"
FILES:${PN} += "${libdir}/libaggregate-sensor.so.0.1 \
                ${PYTHON_SITEPACKAGES_DIR}/aggregate_sensor.py"
