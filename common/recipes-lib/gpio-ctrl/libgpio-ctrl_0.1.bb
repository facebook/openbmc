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

SUMMARY = "GPIO Control Library"
DESCRIPTION = "GPIO library for applications on openbmc kernel 4.1 or higher"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://gpio.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

S = "${WORKDIR}"
SRC_URI = "file://gpio.c \
           file://gpio_int.h \
           file://gpio_sysfs.c \
           file://gpiochip.c \
           file://gpiochip_aspeed.c \
           file://libgpio.h \
           file://Makefile \
          "

CFLAGS += "-Wall -Werror "
LDFLAGS += " -lmisc-utils"

DEPENDS += "libmisc-utils obmc-i2c"
RDEPENDS_${PN} += " libmisc-utils"
DEPENDS_append = " libmisc-utils"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libgpio-ctrl.so ${D}${libdir}/libgpio-ctrl.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 libgpio.h ${D}${includedir}/openbmc/libgpio.h
}

FILES_${PN} = "${libdir}/libgpio-ctrl.so"
FILES_${PN}-dev = "${includedir}/openbmc/libgpio.h"
