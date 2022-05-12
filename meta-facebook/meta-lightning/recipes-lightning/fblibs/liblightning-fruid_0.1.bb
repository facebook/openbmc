# Copyright 2015-present Facebook. All Rights Reserved.
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

SUMMARY = "Lightning Fruid Library"
DESCRIPTION = "library for reading all Lightning fruids"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://lightning_fruid.c;beginline=6;endline=18;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://lightning_fruid \
          "

DEPENDS += " liblightning-common "

S = "${WORKDIR}/lightning_fruid"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 liblightning_fruid.so ${D}${libdir}/liblightning_fruid.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 lightning_fruid.h ${D}${includedir}/facebook/lightning_fruid.h
}

FILES:${PN} = "${libdir}/liblightning_fruid.so"
FILES:${PN}-dev = "${includedir}/facebook/lightning_fruid.h"
RDEPENDS:${PN} += " liblightning-common "
