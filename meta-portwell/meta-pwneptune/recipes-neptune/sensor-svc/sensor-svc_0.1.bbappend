# Copyright 2017-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI =+ "file://setup-sensor-svcd.sh \
            file://run-sensor-svcd.sh \
           "

S = "${WORKDIR}"

LDFLAGS =+ "-lgpio -lsensor-svc-pal -lkv -lipmb -lgpio -lobmc-pal"
DEPENDS =+ "update-rc.d-native libgpio libsensor-svc-pal"
RDEPENDS_${PN} += "libgpio"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/sensor-svcd
  install -d ${D}${sysconfdir}/sensor-svcd

  install -m 755 setup-sensor-svcd.sh ${D}${sysconfdir}/init.d/setup-sensor-svcd.sh
  install -m 755 run-sensor-svcd.sh ${D}${sysconfdir}/sv/sensor-svcd/run
  update-rc.d -r ${D} setup-sensor-svcd.sh start 91 5 .
}

# Inhibit complaints about .debug directories for the sensor-svcd binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
