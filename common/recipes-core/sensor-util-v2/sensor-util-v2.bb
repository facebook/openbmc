# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "DBus based Sensor Utility"
DESCRIPTION = "Util for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://SensorUtilV2.cpp;beginline=4;endline=18;md5=6d800d1c02e2ddf19e5ead261943b73b"

LOCAL_URI = " \
    file://Makefile \
    file://SensorUtilV2.cpp \
    "

binfiles = "sensor-util-v2"

LDFLAGS =+ "-lglib-2.0 -lgio-2.0"
DEPENDS =+ "glib-2.0"
RDEPENDS:${PN} =+ "glib-2.0"

pkgdir = "sensor-util-v2"

export SINC = "${STAGING_INCDIR}"
export SLIB = "${STAGING_LIBDIR}"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 sensor-util-v2 ${dst}/sensor-util-v2
  ln -snf ../fbpackages/${pkgdir}/sensor-util-v2 ${bin}/sensor-util-v2
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/sensor-util-v2 ${prefix}/local/bin"
