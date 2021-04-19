# Copyright 2015-present Facebook. All Rights Reserved.
SUMMARY = "Setup GPIO when BMC boot up"
DESCRIPTION = "Set and export GPIO"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://setup-gpio.c;beginline=8;endline=20;md5=8e8a5829be6e215cdbf65cac2aa6ddc4"

inherit meson

SRC_URI = "file://setup-gpio.c \
           file://meson.build \
           file://setup-gpio.sh \
          "
S = "${WORKDIR}"

DEPENDS += "libpal libgpio-ctrl libfby2-common libphymem"
DEPENDS += "update-rc.d-native"
RDEPENDS_${PN} += "libpal libgpio-ctrl libfby2-common libphymem"

do_install_append() {
  install -d ${D}${sysconfdir}/init.d
  install -m 755 ${S}/setup-gpio.sh ${D}${sysconfdir}/init.d/setup-gpio.sh
  update-rc.d -r ${D} setup-gpio.sh start 50 S .
}

