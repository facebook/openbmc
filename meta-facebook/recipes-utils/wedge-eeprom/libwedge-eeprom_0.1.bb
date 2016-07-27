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
SUMMARY = "Wedge EEPROM Library"
DESCRIPTION = "library for wedge eeprom"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://wedge_eeprom.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://lib \
          "

DEPENDS += "liblog"

S = "${WORKDIR}/lib"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libwedge_eeprom.so ${D}${libdir}/libwedge_eeprom.so

    install -d ${D}${includedir}/facebook
    install -m 0644 wedge_eeprom.h ${D}${includedir}/facebook/wedge_eeprom.h
}

FILES_${PN} = "${libdir}/libwedge_eeprom.so"
FILES_${PN}-dev = "${includedir}/facebook/wedge_eeprom.h"
