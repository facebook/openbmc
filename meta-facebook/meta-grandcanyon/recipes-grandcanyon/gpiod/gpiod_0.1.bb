# Copyright 2021-present Facebook. All Rights Reserved.

SUMMARY = "GPIO Status Monitoring Daemon" 
DESCRIPTION = "Daemon for monitoring BIC GPIO and IO expander GPIO by polling"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://gpiod.c;beginline=4;endline=16;md5=b66b777f082370423b0fa6f12a3dc4db"

inherit meson pkgconfig

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://meson.build \
    file://gpiod.c \
    file://setup-gpiod.sh \
    file://run-gpiod.sh \
    file://clear-hsc-fault.sh \
    "

DEPENDS += " libpal update-rc.d-native libgpio-ctrl libfbgc-common libfbgc-gpio libobmc-i2c"

LDFLAGS:append = " -lexp"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/gpiod
  install -m 755 ${UNPACKDIR}/setup-gpiod.sh ${D}${sysconfdir}/init.d/setup-gpiod.sh
  install -m 755 ${UNPACKDIR}/run-gpiod.sh ${D}${sysconfdir}/sv/gpiod/run
  install -m 755 ${UNPACKDIR}/clear-hsc-fault.sh ${D}${sysconfdir}/init.d/clear-hsc-fault.sh
  update-rc.d -r ${D} clear-hsc-fault.sh start 89 5 .
  update-rc.d -r ${D} setup-gpiod.sh start 90 5 .
}
