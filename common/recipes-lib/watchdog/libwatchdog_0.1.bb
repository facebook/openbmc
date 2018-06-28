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

SUMMARY = "Watchdog Control Library"
DESCRIPTION = "library for controlling watchdog timer in kernel 4.1"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://watchdog.c;beginline=4;endline=18;md5=4008d12a82cf78c337e6d8df3db6ff86"

SRC_URI = "file://Makefile \
           file://watchdog.c \
           file://watchdog.h \
          "

S = "${WORKDIR}"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libwatchdog.so ${D}${libdir}/libwatchdog.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 watchdog.h ${D}${includedir}/openbmc/watchdog.h
}

FILES_${PN} = "${libdir}/libwatchdog.so"
FILES_${PN}-dev = "${includedir}/openbmc/watchdog.h"
