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

SUMMARY = "I2C Device Access Library"
DESCRIPTION = "Library for accessing I2C bus and devices"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://obmc-i2c.h;beginline=7;endline=19;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

inherit meson pkgconfig

LOCAL_URI = " \
    file://obmc-i2c.h \
    file://i2c_cdev.c \
    file://i2c_cdev.h \
    file://i2c_core.h \
    file://i2c_device.c \
    file://i2c_device.h \
    file://i2c_mslave.c \
    file://i2c_mslave.h \
    file://i2c_sysfs.c \
    file://i2c_sysfs.h \
    file://smbus.h \
    file://meson.build \
    "

DEPENDS += "libmisc-utils liblog"
RDEPENDS:${PN} += "libmisc-utils"

