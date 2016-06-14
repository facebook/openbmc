# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
#

SUMMARY = "EEPROM tool Utilities"
DESCRIPTION = "Util for EEPROM tool"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mkeeprom.c;beginline=4;endline=13;md5=03e4e5ed5d6d17edb790a6bcab2eaf6a"

SRC_URI = "file://mkeeprom.c \
		file://mkeeprom.h \
		file://fbsettings.txt \
		file://Makefile \
          "
S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 mkeeprom ${D}${bindir}/mkeeprom
	install -d ${D}${sysconfdir}
	install -m 0644 fbsettings.txt ${D}${sysconfdir}/fbsettings.txt
}

FILES_${PN} = "${bindir}"
FILES_${PN} += "${sysconfdir}"

# Inhibit complaints about .debug directories

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
