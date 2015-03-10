# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "PowerOne EEPROM Utilities"
DESCRIPTION = "Util for PowerOne eeprom"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://po-eeprom.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://po-eeprom.c \
	   file://Makefile \
          "

S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 po-eeprom ${D}${bindir}/po-eeprom
}

FILES_${PN} = "${bindir}"

# Inhibit complaints about .debug directories

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
