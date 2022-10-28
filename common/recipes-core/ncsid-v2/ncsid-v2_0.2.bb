# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "ncsid tx/rx daemon"
DESCRIPTION = "The NC-SI daemon to receive/transmit messages"
SECTION = "base"
PR = "r2"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ncsid.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://meson.build \
    file://ncsid.c \
    file://setup-ncsid.sh \
    file://enable-aen.sh \
    file://run-ncsid.sh \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "ncsid"

DEPENDS += "update-rc.d-native libpal libncsi obmc-libpldm libkv libnl-wrapper"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/ncsid
  install -d ${D}${sysconfdir}/ncsid
  install -m 755 ${S}/setup-ncsid.sh ${D}${sysconfdir}/init.d/setup-ncsid.sh
  install -m 755 ${S}/run-ncsid.sh ${D}${sysconfdir}/sv/ncsid/run
  install -m 755 ${S}/enable-aen.sh ${D}${bindir}/enable-aen.sh
  update-rc.d -r ${D} setup-ncsid.sh start 91 5 .
}
