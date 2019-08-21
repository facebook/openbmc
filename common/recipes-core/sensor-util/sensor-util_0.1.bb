# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Sensor Utility"
DESCRIPTION = "Util for reading various sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sensor-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://Makefile \
           file://sensor-util.c \
          "
S = "${WORKDIR}"

binfiles = "sensor-util"

LDFLAGS = " -ljansson"
DEPENDS =+ " libsdr libpal libaggregate-sensor jansson"
RDEPENDS_${PN} =+ "libsdr libpal libaggregate-sensor jansson"

pkgdir = "sensor-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 sensor-util ${dst}/sensor-util
  ln -snf ../fbpackages/${pkgdir}/sensor-util ${bin}/sensor-util
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/sensor-util ${prefix}/local/bin"
