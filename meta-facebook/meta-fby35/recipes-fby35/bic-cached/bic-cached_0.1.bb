# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Bridge IC Cache Tool"
DESCRIPTION = "Tool to provide Bridge IC Cache information."
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-or-later;md5=fed54355545ffd980b814dab4a3b312c"

inherit meson pkgconfig
inherit legacy-packages

LOCAL_URI = " \
    file://meson.build \
    file://setup-bic-cached.sh \
    file://bic-cached.cpp \
    "

DEPENDS = " libfby35-common libbic libpal update-rc.d-native"
RDEPENDS:${PN} = " bash"

pkgdir = "bic-cached"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -m 755 ${S}/setup-bic-cached.sh ${D}${sysconfdir}/init.d/setup-bic-cached.sh
  update-rc.d -r ${D} setup-bic-cached.sh start 66 5 .
}
