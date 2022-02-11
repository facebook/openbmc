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

SUMMARY = "Wedge400 PSUs Library"
DESCRIPTION = "library for PSUs"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://agc032a-psu.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://agc032a_psu \
          "

LDFLAGS = "-lfruid -lpal"

DEPENDS += "libfruid libpal"
RDEPENDS_${PN} += "libfruid libpal"

S = "${WORKDIR}/agc032a_psu"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libagc032a-psu.so ${D}${libdir}/libagc032a-psu.so

    install -d ${D}${includedir}/facebook
    install -m 0644 agc032a-psu.h ${D}${includedir}/facebook/agc032a-psu.h
}

FILES_${PN} = "${libdir}/libagc032a-psu.so"
FILES_${PN}-dev = "${includedir}/facebook/agc032a-psu.h"
