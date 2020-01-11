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

SUMMARY = "Wedge400 PEMs Library"
DESCRIPTION = "library for PEMs"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://wedge400-pem.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://wedge400_pem \
          "

LDFLAGS = "-lfruid -lpal -lobmc-i2c -lobmc-pmbus"

DEPENDS += "libfruid libpal libobmc-i2c libobmc-pmbus"
RDEPENDS_${PN} += "libfruid libpal libobmc-i2c libobmc-pmbus"

S = "${WORKDIR}/wedge400_pem"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libwedge400-pem.so ${D}${libdir}/libwedge400-pem.so

    install -d ${D}${includedir}/facebook
    install -m 0644 wedge400-pem.h ${D}${includedir}/facebook/wedge400-pem.h
}

FILES_${PN} = "${libdir}/libwedge400-pem.so"
FILES_${PN}-dev = "${includedir}/facebook/wedge400-pem.h"
