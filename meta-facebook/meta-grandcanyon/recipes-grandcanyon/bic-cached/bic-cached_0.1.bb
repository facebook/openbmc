# Copyright 2020-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Tool"
DESCRIPTION = "Tool to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bic-cached.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

DEPENDS:append = " libbic libpal update-rc.d-native "

LOCAL_URI = " \
    file://meson.build \
    file://setup-bic-cached.sh \
    file://bic-cached.c \
    "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 ${S}/setup-bic-cached.sh ${D}${sysconfdir}/init.d/setup-bic-cached.sh
  update-rc.d -r ${D} setup-bic-cached.sh start 66 5 .
}

