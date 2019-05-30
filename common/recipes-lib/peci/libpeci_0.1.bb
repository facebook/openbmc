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
SUMMARY = "PECI Library"
DESCRIPTION = "library for PECI"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://peci.c;beginline=8;endline=20;md5=435eababecd3f367d90616c70e27bdd6"

SRC_URI = "file://Makefile \
           file://peci.c \
           file://peci.h \
          "

S = "${WORKDIR}"

do_install() {
  install -d ${D}${libdir}
  install -m 0644 libpeci.so ${D}${libdir}/libpeci.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 peci.h ${D}${includedir}/openbmc/peci.h
}

FILES_${PN} = "${libdir}/libpeci.so"
FILES_${PN}-dev = "${includedir}/openbmc/peci.h"
