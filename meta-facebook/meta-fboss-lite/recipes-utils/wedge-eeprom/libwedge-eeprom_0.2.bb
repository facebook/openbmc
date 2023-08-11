# Copyright 2023-present Mate Platform. All Rights Reserved.
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

SUMMARY = "Wedge EEPROM Utilities"
DESCRIPTION = "Util for BMC-lite wedge eeprom"
SECTION = "base"
PR = "r0"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://WeLibApi.cpp;beginline=1;endline=12;md5=6363afb5f510280e03b900c00cadbb10"

inherit meson pkgconfig
inherit python3native
inherit python3-dir

CFLAGS += "-Wall -Werror"

SRC_URI = "file://lib \
          "
LDFLAGS += "-llog -lobmc-i2c"
DEPENDS += "nlohmann-json python3-setuptools liblog libobmc-i2c"
RDEPENDS:${PN} = "python3-core bash liblog libobmc-i2c"

S = "${WORKDIR}/lib"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libwedge-eeprom.so ${D}${libdir}/libwedge-eeprom.so
    install -m 0644 libwedge-eeprom.so.0 ${D}${libdir}/libwedge-eeprom.so.0

    install -d ${D}${includedir}/facebook
    install -m 0644 ${S}/WeutilInterface.h ${D}${includedir}/facebook/WeutilInterface.h

    install -d ${D}/${sysconfdir}/weutil
    install -m 0644 ${S}/eeprom.json ${D}/${sysconfdir}/weutil/eeprom.json
}

FILES:${PN} = "${sysconfdir}/weutil/eeprom.json"
FILES:${PN} += "${libdir}/libwedge-eeprom.so ${libdir}/libwedge-eeprom.so.0"
FILES:${PN}-dev = "${includedir}/facebook/WeutilInterface.h "

