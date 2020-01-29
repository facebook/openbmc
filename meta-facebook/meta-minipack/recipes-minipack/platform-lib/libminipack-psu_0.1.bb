# Copyright 2018-present Facebook. All Rights Reserved.
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

SUMMARY = "Minipack PSUs Library"
DESCRIPTION = "library for PSUs"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://minipack-psu.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://minipack_psu \
          "

LDFLAGS = "-lfruid -lpal -lobmc-i2c -llog"

DEPENDS += "libfruid libpal libobmc-i2c liblog"
RDEPENDS_${PN} += "libfruid libpal libobmc-i2c liblog"

S = "${WORKDIR}/minipack_psu"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libminipack-psu.so ${D}${libdir}/libminipack-psu.so

    install -d ${D}${includedir}/facebook
    install -m 0644 minipack-psu.h ${D}${includedir}/facebook/minipack-psu.h
}

FILES_${PN} = "${libdir}/libminipack-psu.so"
FILES_${PN}-dev = "${includedir}/facebook/minipack-psu.h"
