# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "eMMC Daemon"
DESCRIPTION = "Daemon to monitor eMMC status "
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://emmcd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson

RDEPENDS_${PN} += "liblog libmisc-utils"
DEPENDS_append = " update-rc.d-native liblog libmisc-utils"

SRC_URI = " \
        file://emmcd.c \
        file://meson.build \
        file://run-emmcd.sh \
        file://setup-emmcd.sh \
        "

S = "${WORKDIR}"

pkgdir = "emmcd"
inherit legacy-packages

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/emmcd
  install -m 755 ${S}/setup-emmcd.sh ${D}${sysconfdir}/init.d/setup-emmcd.sh
  install -m 755 ${S}/run-emmcd.sh ${D}${sysconfdir}/sv/emmcd/run
  update-rc.d -r ${D} setup-emmcd.sh start 99 5 .
}
