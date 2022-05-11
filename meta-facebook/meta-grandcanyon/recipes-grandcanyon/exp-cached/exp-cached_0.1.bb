# Copyright 2020-present Facebook. All Rights Reserved.

SUMMARY = "SCC Expander Cache Daemon"
DESCRIPTION = "Daemon to provide SCC Expander Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://exp-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


DEPENDS:append = "libpal libexp update-rc.d-native"
RDEPENDS:${PN} += "libpal libexp"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://setup-exp-cached.sh \
    file://exp-cached.c \
    "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 ${S}/setup-exp-cached.sh ${D}${sysconfdir}/init.d/setup-exp-cached.sh
  update-rc.d -r ${D} setup-exp-cached.sh start 71 5 .
}

