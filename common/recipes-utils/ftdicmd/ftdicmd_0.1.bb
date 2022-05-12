#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
#

SUMMARY = "FTDI Utilitys to use libftdi for configuration"
DESCRIPTION = " \
    FTDI Utilitys will use libftdi for specific application \
    ftdi_control for FTDI EEPROM configuration \
    ftdi_bitbake for using FTDI port as GPIO \
"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Makefile;beginline=4;endline=17;md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://Makefile \
    file://cmd-bitbang.c \
    file://cmd-control.c \
    file://cmd-common.c \
    file://cmd-common.h \
    file://ftdi-bitbang.c \
    file://ftdi-bitbang.h \
    "

RDEPENDS:${PN} += "libusb1 libftdi"
DEPENDS:append = "libusb1 libftdi"
LDFLAGS += "-L . -l usb-1.0 -l ftdi1 "


binfiles = "ftdi_bitbang ftdi_control"

pkgdir = "ftdicmd"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 ftdi_bitbang ${dst}/ftdi_bitbang
  install -m 755 ftdi_control ${dst}/ftdi_control
  ln -snf ../fbpackages/${pkgdir}/ftdi_bitbang ${bin}/ftdi_bitbang
  ln -snf ../fbpackages/${pkgdir}/ftdi_control ${bin}/ftdi_control
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"
FILES:${PN} = "${FBPACKAGEDIR}/ftdicmd ${prefix}/local/bin "
