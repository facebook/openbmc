# Copyright 2014-present Facebook. All Rights Reserved.
SUMMARY = "Wedge EEPROM Utilities"
DESCRIPTION = "Util for wedge eeprom"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://weutil.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://utils \
          "

S = "${WORKDIR}/utils"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 weutil ${D}${bindir}/weutil
}

DEPENDS += "libwedge-eeprom"

FILES_${PN} = "${bindir}"
