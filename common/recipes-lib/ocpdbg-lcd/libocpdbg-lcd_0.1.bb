# Copyright 2017-present Facebook. All Rights Reserved.
SUMMARY = "Library providing common features for the LCD USB debug card"
DESCRIPTION = "Library providing common features for the LCD USB debug card"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ocp-dbg-lcd.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://meson.build \
           file://ocp-dbg-lcd.c \
           file://ocp-dbg-lcd.h \
          "

S = "${WORKDIR}"

DEPENDS = " libipmb libipmi libobmc-i2c"
RDEPENDS_${PN} = "libipmb libipmi libobmc-i2c"

inherit meson
