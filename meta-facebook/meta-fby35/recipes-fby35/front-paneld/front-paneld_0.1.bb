# Copyright 2015-present Facebook. All Rights Reserved.

SUMMARY = "Front Panel Control Daemon"
DESCRIPTION = "Daemon to monitor and control the front panel "
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://front-paneld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS += "libfby35-common libpal libkv update-rc.d-native"

LOCAL_URI = " \
    file://meson.build \
    file://setup-front-paneld.sh \
    file://front-paneld.c \
    file://run-front-paneld.sh \
    "

inherit meson pkgconfig
inherit legacy-packages

pkgdir = "front-paneld"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/front-paneld
  install -d ${D}${sysconfdir}/front-paneld
  install -m 755 ${S}/setup-front-paneld.sh ${D}${sysconfdir}/init.d/setup-front-paneld.sh
  install -m 755 ${S}/run-front-paneld.sh ${D}${sysconfdir}/sv/front-paneld/run
  update-rc.d -r ${D} setup-front-paneld.sh start 67 5 .
}

