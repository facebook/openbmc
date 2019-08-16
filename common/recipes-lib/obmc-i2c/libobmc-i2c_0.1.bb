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
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://obmc-i2c.h;beginline=7;endline=19;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

SRC_URI = "file://obmc-i2c.h \
           file://i2c_cdev.c \
           file://i2c_cdev.h \
           file://i2c_core.h \
           file://i2c_mslave.c \
           file://i2c_mslave.h \
           file://i2c_sysfs.c \
           file://i2c_sysfs.h \
           file://smbus.h \
           file://Makefile \
          "

S = "${WORKDIR}"

LDFLAGS += "-lmisc-utils -llog"
DEPENDS += "libmisc-utils liblog"
RDEPENDS_${PN} += "libmisc-utils liblog"

#
# Below are the public header files exported by "libobmc-i2c": Callers
# can either include "obmc-i2c.h" which contains all the library APIs,
# or they can also include specific header files if they are clear which
# parts are needed.
#
I2C_HEADER_FILES = " \
    obmc-i2c.h \
    i2c_cdev.h \
    i2c_core.h \
    i2c_mslave.h \
    i2c_sysfs.h \
    smbus.h \
"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libobmc-i2c.so ${D}${libdir}/libobmc-i2c.so

    install -d ${D}${includedir}/openbmc
    for f in ${I2C_HEADER_FILES}; do
        install -m 0644 ${f} ${D}${includedir}/openbmc/${f}
    done
}

FILES_${PN} = "${libdir}/libobmc-i2c.so"
FILES_${PN}-dev = "${includedir}/openbmc/*.h"
