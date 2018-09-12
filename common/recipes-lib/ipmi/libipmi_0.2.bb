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
SUMMARY = "IPMI Client Library"
DESCRIPTION = "library for IPMI Client"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://ipmi.c;beginline=8;endline=20;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

SRC_URI = "file://Makefile \
           file://ipmi.c \
           file://ipmi.h \
          "

DEPENDS += "libipc"
RDEPENDS_${PN} += "libipc"

S = "${WORKDIR}"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libipmi.so ${D}${libdir}/libipmi.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 ipmi.h ${D}${includedir}/openbmc/ipmi.h
}

FILES_${PN} = "${libdir}/libipmi.so"
FILES_${PN}-dev = "${includedir}/openbmc/ipmi.h"
