# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing support for sensor correction"
DESCRIPTION = "Library providing support for sensor correction"
SECTION = "libs"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sensor-correction.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://CMakeLists.txt \
           file://sensor-correction.h \
           file://sensor-correction.c \
           file://sensor-correction-conf.json \
          "
SENSOR_CORR_CONFIG = "sensor-correction-conf.json"
S = "${WORKDIR}"

do_install_append() {
  install -d ${D}${sysconfdir}
  for f in ${SENSOR_CORR_CONFIG}; do
    install -m 644 ${WORKDIR}/$f ${D}${sysconfdir}/$f
  done
}

DEPENDS =+ "libedb jansson"
RDEPENDS_${PN} += "libedb jansson"

inherit cmake
