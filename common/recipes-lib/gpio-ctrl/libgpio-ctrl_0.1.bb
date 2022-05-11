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

inherit meson pkgconfig python3-dir
inherit ptest-meson

LOCAL_URI = " \
    file://meson.build \
    file://gpio.c \
    file://gpio_int.h \
    file://gpio_sysfs.c \
    file://gpiochip.c \
    file://gpiochip_aspeed.c \
    file://libgpio.h \
    file://libgpio.hpp \
    file://libgpio.py \
    file://gpio_test.cpp \
    "

DEPENDS += "libmisc-utils libobmc-i2c gtest gmock"
RDEPENDS:${PN} += " libmisc-utils libobmc-i2c python3-core"
RDEPENDS:${PN}-ptest += "libmisc-utils libobmc-i2c"

do_install:append() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/libgpio.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}
FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}/libgpio.py"
