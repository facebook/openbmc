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

SUMMARY = "Gibraltar I2C Tool"
DESCRIPTION = "I2C Utility to access with spacial protocol for Gibraltar Switch"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://gbi2ctool.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"

SRC_URI = "file://gbi2ctool.c \
           file://Makefile \
           "

S = "${WORKDIR}"

binfiles = "gbi2ctool"

pkgdir = "gbi2ctool"


do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 gbi2ctool ${dst}/gbi2ctool
  ln -snf ../fbpackages/${pkgdir}/gbi2ctool ${bin}/gbi2ctool
}

LDFLAGS += " -lobmc-i2c"
DEPENDS += "libobmc-i2c"
RDEPENDS_${PN} += " libobmc-i2c"

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES_${PN} = "${FBPACKAGEDIR}/gbi2ctool ${prefix}/local/bin ${sysconfdir} "

