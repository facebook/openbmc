# Copyright 2020-present Facebook. All Rights Reserved.
SUMMARY = "MAC Utility"
DESCRIPTION = "Util to get/set MAC address from/to EEPROM"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mac-util.cpp;beginline=4;endline=16;md5=94a0865391a6425c9dcee589aa6888d5"

SRC_URI += "file://mac-util.cpp \
           file://Makefile \
          "
SRC_URI_append_${MACHINE} = "file://${MACHINE}/eeprom.h "
CXXFLAGS_prepend_${MACHINE} += "-I${MACHINE}"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 mac-util ${D}${bindir}/mac-util
}

DEPENDS += "cli11 "

FILES_${PN} = "${bindir}"
