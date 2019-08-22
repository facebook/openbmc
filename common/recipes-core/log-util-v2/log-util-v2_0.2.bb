# Copyright 2015-present Facebook. All Rights Reserved.
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
SUMMARY = "Log Utility"
DESCRIPTION = "Utility to parse and display logs."
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://log-util-v2.cpp;beginline=5;endline=18;md5=ff9a2ba58fa5b39d3d3dcb7c42e26496"


SRC_URI = "file://Makefile \
           file://log-util-v2.cpp \
          "

S = "${WORKDIR}"
binfiles = "log-util-v2"

CFLAGS += " -lpal "

pkgdir = "log-util-v2"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 log-util-v2 ${dst}/log-util-v2
  ln -snf ../fbpackages/${pkgdir}/log-util-v2 ${bin}/log-util
}

DEPENDS += "libpal"
RDEPENDS_${PN} += "libpal"


FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/log-util-v2 ${prefix}/local/bin"

