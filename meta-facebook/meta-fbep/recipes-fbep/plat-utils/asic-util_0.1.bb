# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "ASIC Utility"
DESCRIPTION = "Util to communicate wiht GPU on ASIC"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://asic-util.cpp;beginline=4;endline=16;md5=94a0865391a6425c9dcee589aa6888d5"

SRC_URI = "file://asic-util \
          "

S = "${WORKDIR}/asic-util"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 asic-util ${D}${bindir}/asic-util
}

DEPENDS += "libpal libasic libgpio-ctrl libkv cli11 "
RDEPENDS:${PN} += "libpal libasic libgpio-ctrl libkv "
LDFLAGS += "-lpal -lasic -lgpio-ctrl -lkv "

FILES:${PN} = "${bindir}"
