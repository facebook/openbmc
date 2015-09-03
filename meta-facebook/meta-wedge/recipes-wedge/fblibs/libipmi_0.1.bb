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
SUMMARY = "Wedge IPMI Client Library"
DESCRIPTION = "library for Wedge IPMI Client"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmi.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"


SRC_URI = "file://ipmi \
          "

S = "${WORKDIR}/ipmi"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libipmi.so ${D}${libdir}/libipmi.so

    install -d ${D}${includedir}/facebook
    install -m 0644 ipmi.h ${D}${includedir}/facebook/ipmi.h
}

FILES_${PN} = "${libdir}/libipmi.so"
FILES_${PN}-dev = "${includedir}/facebook/ipmi.h"
