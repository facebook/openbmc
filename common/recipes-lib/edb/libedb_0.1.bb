# Copyright 2016-present Facebook. All Rights Reserved.
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

SUMMARY = "Embedded Database Library"
DESCRIPTION = "library for storing key-value pairs"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://edb.c;beginline=4;endline=16;md5=8d76ccaa6a7a0995dadc51f3d016e179"

SRC_URI = "file://edb.c \
           file://edb.h \
           file://Makefile \
          "

S = "${WORKDIR}"

do_install() {
	  install -d ${D}${libdir}
    install -m 0644 libedb.so ${D}${libdir}/libedb.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 edb.h ${D}${includedir}/openbmc/edb.h
}

FILES_${PN} = "${libdir}/libedb.so"
FILES_${PN}-dev = "${includedir}/openbmc/edb.h"
