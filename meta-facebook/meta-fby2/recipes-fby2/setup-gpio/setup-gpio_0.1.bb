# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Setup GPIO when BMC boot up"
DESCRIPTION = "Set and export GPIO"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://setup-gpio.c;beginline=8;endline=20;md5=8e8a5829be6e215cdbf65cac2aa6ddc4"

inherit meson pkgconfig

LOCAL_URI = " \
    file://setup-gpio.c \
    file://meson.build \
    file://setup-gpio.sh \
    "

DEPENDS += "libpal libgpio-ctrl libfby2-common libphymem"
DEPENDS += "update-rc.d-native"
RDEPENDS:${PN} += "libpal libgpio-ctrl libfby2-common libphymem"

do_install:append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ${S}/setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 50 S .
}

