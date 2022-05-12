# Copyright 2019-present Facebook. All Rights Reserved.
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


SUMMARY = "Common PEMs Library"
DESCRIPTION = "library for PEMs"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://pem.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://pem.c \
    file://pem.h \
    file://pem-platform.c \
    file://pem-platform.h \
    file://Makefile \
    "

LDFLAGS = "-lfruid -lpal -lobmc-i2c -lobmc-pmbus -lwedge_eeprom"

DEPENDS += "libfruid libpal libobmc-i2c libobmc-pmbus"
RDEPENDS:${PN} += "libfruid libpal libobmc-i2c libobmc-pmbus"


do_install() {
    install -d ${D}${libdir}
    install -m 0644 libpem.so ${D}${libdir}/libpem.so

    install -d ${D}${includedir}/facebook
    install -m 0644 pem.h ${D}${includedir}/facebook/pem.h
}

FILES:${PN} = "${libdir}/libpem.so"
FILES:${PN}-dev = "${includedir}/facebook/pem.h"
