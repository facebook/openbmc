# Copyright 2017-present Facebook. All Rights Reserved.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI =+ "file://setup-platform-svcd.sh \
            file://run-platform-svcd.sh \
            file://platform-svcd-config.json \
           "

S = "${WORKDIR}"

DEPENDS =+ "update-rc.d-native"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/platform-svcd
  install -d ${D}${sysconfdir}/platform-svcd

  install -m 755 setup-platform-svcd.sh ${D}${sysconfdir}/init.d/setup-platform-svcd.sh
  install -m 755 run-platform-svcd.sh ${D}${sysconfdir}/sv/platform-svcd/run
  install -m 644 platform-svcd-config.json ${D}${sysconfdir}/platform-svcd-config.json
  update-rc.d -r ${D} setup-platform-svcd.sh start 91 5 .
}
