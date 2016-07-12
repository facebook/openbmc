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

SUMMARY = "CPLD Update Utilities"
DESCRIPTION = "Util for CPLD update"
SECTION = "base"
PR = "r1"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ispvm_ui.c;beginline=5;endline=13;md5=2c3382680a7be213f70f172683fc93c4"

SRC_URI = "file://ispvm_ui.c \
		file://ivm_core.c \
		file://hardware.c \
		file://vmopcode.h \
		file://i2c-dev.h \
		file://Makefile \
          "

S = "${WORKDIR}"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ispvm ${D}${bindir}/ispvm
}

FILES_${PN} = "${bindir}"

# Inhibit complaints about .debug directories

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
