# Copyright 2021-present Facebook. All Rights Reserved.

SUMMARY = "GPIO Status Monitoring Daemon" 
DESCRIPTION = "Daemon for monitoring BIC GPIO and IO expander GPIO by polling"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://gpiod.c;beginline=4;endline=16;md5=b66b777f082370423b0fa6f12a3dc4db"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://gpiod.c \
    file://setup-gpiod.sh \
    file://run-gpiod.sh \
    "

DEPENDS += " libpal update-rc.d-native libgpio-ctrl libfbgc-common libfbgc-gpio "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/gpiod
  install -m 755 ${S}/setup-gpiod.sh ${D}${sysconfdir}/init.d/setup-gpiod.sh
  install -m 755 ${S}/run-gpiod.sh ${D}${sysconfdir}/sv/gpiod/run
  update-rc.d -r ${D} setup-gpiod.sh start 90 5 .
}
