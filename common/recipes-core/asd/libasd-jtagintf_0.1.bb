# Copyright 2017-present Facebook. All Rights Reserved.
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

SUMMARY = "JTAG Interface library"
DESCRIPTION = "Library which helps interfacing with the JTAG hardware"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://SoftwareJTAGHandler.c;beginline=6;endline=13;md5=678b5e5a5bd177043fb4177e772c804d"

SRC_URI = "file://interface/SoftwareJTAGHandler.c \
           file://interface/SoftwareJTAGHandler.h \
           file://interface/Makefile \
           "

DEPENDS += "libgpio libpal"

S = "${WORKDIR}/interface"

do_install() {
  install -d ${D}${includedir}/asd
  install -m 0644 SoftwareJTAGHandler.h ${D}${includedir}/asd/SoftwareJTAGHandler.h

  install -d ${D}${libdir}
  install -m 0644 libasd-jtagintf.so ${D}${libdir}/libasd-jtagintf.so
}

FILES_${PN} = "${libdir}/libasd-jtagintf.so"
FILES_${PN}-dev = "${includedir}/asd"
RDEPENDS_${PN} = "libgpio libpal"

