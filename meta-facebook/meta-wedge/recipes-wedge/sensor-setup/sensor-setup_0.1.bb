# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Configure the sensors"
DESCRIPTION = "The script configure sensors"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://sensor-setup.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

DEPENDS_append = " update-rc.d-native"

SRC_URI = "file://sensor-setup.sh \
          "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 sensor-setup.sh ${D}${sysconfdir}/init.d/sensor-setup.sh
  update-rc.d -r ${D} sensor-setup.sh start 90 S .
}

FILES_${PN} = " ${sysconfdir} "
