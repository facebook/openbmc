# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Device driver using GPIO bitbang"
DESCRIPTION = "Various device driver using GPIO bitbang"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bitbang.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://src \
          "

DEPENDS += "fbutils"

S = "${WORKDIR}/src"

do_install() {
  install -d ${D}${bindir}
  install -m 755 spi-bb ${D}${bindir}/spi-bb
  install -m 755 mdio-bb ${D}${bindir}/mdio-bb
}

FILES_${PN} = "${bindir}"
