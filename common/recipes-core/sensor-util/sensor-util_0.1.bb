# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Sensor Utility"
DESCRIPTION = "Util for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://sensor-util.cpp;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

inherit meson pkgconfig
inherit ptest-meson

LOCAL_URI = " \
    file://meson.build \
    file://sensor-util.cpp \
    file://gen_sensor_spec.py \
    file://sensor-history.py \
    "

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/gen_sensor_spec.py ${D}${bindir}/gen_sensor_spec
    install -m 0755 ${S}/sensor-history.py ${D}${bindir}/sensor-history
}

pkgdir = "sensor-util"

DEPENDS =+ " libsdr libpal libaggregate-sensor jansson"
RDEPENDS:${PN} =+ "libsdr libpal libaggregate-sensor jansson python3-core"
FILES:${PN} += "${prefix}/local/bin/sensor-util"


