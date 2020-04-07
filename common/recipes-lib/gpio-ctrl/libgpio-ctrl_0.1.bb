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

BBCLASSEXTEND = "native"

S = "${WORKDIR}"
SRC_URI = "file://gpio.c \
           file://gpio_int.h \
           file://gpio_sysfs.c \
           file://gpiochip.c \
           file://gpiochip_aspeed.c \
           file://libgpio.h \
           file://Makefile \
           file://libgpio.py \
          "

CFLAGS += "-Wall -Werror "
LDFLAGS += " -lmisc-utils -lpthread -lobmc-i2c "

DEPENDS += "libmisc-utils libobmc-i2c"
RDEPENDS_${PN} += " libmisc-utils libobmc-i2c python3-core"

inherit distutils3 python3-dir
distutils3_do_configure(){
    :
}

do_compile() {
  make
}

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libgpio-ctrl.so ${D}${libdir}/libgpio-ctrl.so
    ln -s libgpio-ctrl.so ${D}${libdir}/libgpio-ctrl.so.0

    install -d ${D}${includedir}/openbmc
    install -m 0644 libgpio.h ${D}${includedir}/openbmc/libgpio.h

    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 libgpio.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}

FILES_${PN} = "${libdir}/libgpio-ctrl.so* ${PYTHON_SITEPACKAGES_DIR}/libgpio.py"
FILES_${PN}-dev = "${includedir}/openbmc/libgpio.h"
