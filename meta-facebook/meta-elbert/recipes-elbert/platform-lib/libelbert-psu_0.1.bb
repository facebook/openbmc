# Copyright 2021-present Facebook. All Rights Reserved.
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

SUMMARY = "Elbert PSUs Library"
DESCRIPTION = "library for PSUs"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://elbert-psu.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://elbert_psu \
          "

LDFLAGS = "-lfruid -lpal -lobmc-i2c -llog"

DEPENDS += "libfruid libpal libobmc-i2c liblog"
RDEPENDS:${PN} += "libfruid libpal libobmc-i2c liblog"

S = "${WORKDIR}/elbert_psu"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libelbert-psu.so ${D}${libdir}/libelbert-psu.so

    install -d ${D}${includedir}/facebook
    install -m 0644 elbert-psu.h ${D}${includedir}/facebook/elbert-psu.h
}

FILES:${PN} = "${libdir}/libelbert-psu.so"
FILES:${PN}-dev = "${includedir}/facebook/elbert-psu.h"
