# Copyright 2022-present Facebook. All Rights Reserved.
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
SUMMARY = "FBDarwin Prefdl EEPROM Library"
DESCRIPTION = "library for fbdarwin prefdl eeprom"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://prefdl_eeprom.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://lib \
          "

LDFLAGS += "-llog -lobmc-i2c"
DEPENDS += "liblog libobmc-i2c"
RDEPENDS:${PN} = "liblog libobmc-i2c"

S = "${WORKDIR}/lib"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libprefdl_eeprom.so ${D}${libdir}/libprefdl_eeprom.so

    install -d ${D}${includedir}/facebook
    install -m 0644 prefdl_eeprom.h ${D}${includedir}/facebook/prefdl_eeprom.h
}

FILES:${PN} = "${libdir}/libprefdl_eeprom.so"
FILES:${PN}-dev = "${includedir}/facebook/prefdl_eeprom.h"
