# Copyright 2021-present Facebook. All Rights Reserved.

SUMMARY = "GPIO Monitoring Daemon"
DESCRIPTION = "Daemon for monitoring the gpio signals"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://gpiointrd.c;beginline=4;endline=16;md5=302e73da84735a7814365fd8ab355e2d"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://gpiointrd.c \
    file://setup-gpiointrd.sh \
    file://run-gpiointrd.sh \
    "

DEPENDS += " libpal update-rc.d-native libgpio-ctrl libfbgc-common libfbgc-gpio "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/gpiointrd
  install -m 755 ${S}/setup-gpiointrd.sh ${D}${sysconfdir}/init.d/setup-gpiointrd.sh
  install -m 755 ${S}/run-gpiointrd.sh ${D}${sysconfdir}/sv/gpiointrd/run
  update-rc.d -r ${D} setup-gpiointrd.sh start 91 5 .
}
