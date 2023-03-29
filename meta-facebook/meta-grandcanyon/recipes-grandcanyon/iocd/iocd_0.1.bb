# Copyright 2021-present Facebook. All Rights Reserved.

SUMMARY = "IOC temperature and firmware version monitoring daemon"
DESCRIPTION = "Daemon for getting IOC temperature and firmware version"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://iocd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

inherit meson pkgconfig

LOCAL_URI = " \
    file://meson.build \
    file://iocd.c \
    file://iocd.h \
    file://setup-iocd.sh \
    file://run-iocd_11.sh \
    file://run-iocd_12.sh \
    "

LDFLAGS += "-lpal -lobmc-i2c -lkv"
DEPENDS += " libpal update-rc.d-native libfbgc-common libobmc-i2c libipc libkv "

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/iocd_11
  install -d ${D}${sysconfdir}/sv/iocd_12
  install -m 755 ${S}/setup-iocd.sh ${D}${sysconfdir}/init.d/setup-iocd.sh
  install -m 755 ${S}/run-iocd_11.sh ${D}${sysconfdir}/sv/iocd_11/run
  install -m 755 ${S}/run-iocd_12.sh ${D}${sysconfdir}/sv/iocd_12/run
  update-rc.d -r ${D} setup-iocd.sh start 95 5 .
}

