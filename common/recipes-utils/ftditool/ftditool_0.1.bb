# Copyright 2021-present Facebook. All Rights Reserved.
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

SUMMARY = "FDTI USB Tool"
DESCRIPTION = "USB Utility to access with spacial protocol for FTDI232 chips"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://ftditool.c;beginline=5;endline=16;md5=69348da7e13c557a246cf7e5b163ea27"

LOCAL_URI = " \
    file://ftditool.c \
    file://Makefile \
    file://ftdi-eeprom.h \
    "

RDEPENDS:${PN} += "libusb1"
DEPENDS:append = "libusb1"
LDFLAGS += "-L . -l usb-1.0 "


binfiles = "ftditool"

pkgdir = "ftditool"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ftditool ${dst}/ftditool
  ln -snf ../fbpackages/${pkgdir}/ftditool ${bin}/ftditool
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/ftditool ${prefix}/local/bin ${sysconfdir} "
