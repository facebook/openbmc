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

SUMMARY = "Wedge400 GPIO Pin Library"
DESCRIPTION = "library for all gpio pins in wedge400"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://wedge400_gpio.c;beginline=6;endline=18;md5=070623b19848af757119827e6828c575"


SRC_URI = "file://wedge400_gpio \
          "

DEPENDS += " liblog libbic "
LDFLAGS += " -llog -lbic"

S = "${WORKDIR}/wedge400_gpio"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libwedge400_gpio.so ${D}${libdir}/libwedge400_gpio.so

    install -d ${D}${includedir}
    install -d ${D}${includedir}/facebook
    install -m 0644 wedge400_gpio.h ${D}${includedir}/facebook/wedge400_gpio.h
}

FILES_${PN} = "${libdir}/libwedge400_gpio.so"
FILES_${PN}-dev = "${includedir}/facebook/wedge400_gpio.h"
