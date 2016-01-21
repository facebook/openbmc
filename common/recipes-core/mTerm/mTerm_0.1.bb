# Copyright 2014-present Facebook. All Rights Reserved.
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

SUMMARY = "Terminal Multiplexer"
DESCRIPTION = "Util for multiplexing terminal"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://mTerm_server.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

SRC_URI = "file://mTerm_server.c \
           file://mTerm_client.c \
           file://mTerm_helper.c \
           file://mTerm_helper.h \
           file://tty_helper.c \
           file://tty_helper.h \
	         file://Makefile \
          "

S = "${WORKDIR}"

CONS_BIN_FILES = "mTerm_server \
                  mTerm_client \
                 "
pkgdir = "mTerm"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  for f in ${CONS_BIN_FILES}; do
     install -m 755 $f ${dst}/$f
     ln -snf ../fbpackages/${pkgdir}/$f ${bin}/$f
  done
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/mTerm ${prefix}/local/bin"

# Inhibit complaints about .debug directories for the fand binary:

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
