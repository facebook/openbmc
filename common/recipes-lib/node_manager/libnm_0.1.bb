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
SUMMARY = "Node Manager Library"
DESCRIPTION = "library for Node Manager"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://nm.c;beginline=8;endline=20;md5=f5d9c63b6e399f2737960256ca505af9"


SRC_URI = "file://Makefile \
           file://nm.c \
           file://nm.h \
          "

S = "${WORKDIR}"

LDFLAGS += "-lipmb -lipmi"
DEPENDS += "libipmb libipmi"
RDEPENDS_${PN} += "libipmb libipmi"


do_install() {
  install -d ${D}${libdir}
  install -m 0644 libnm.so ${D}${libdir}/libnm.so

  install -d ${D}${includedir}/openbmc
  install -m 0644 nm.h ${D}${includedir}/openbmc/nm.h
}


FILES_${PN} = "${libdir}/libnm.so"
FILES_${PN}-dev = "${includedir}/openbmc/nm.h"
