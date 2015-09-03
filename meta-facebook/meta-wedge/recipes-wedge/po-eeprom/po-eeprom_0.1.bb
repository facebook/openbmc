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
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

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
