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
LIC_FILES_CHKSUM = "file://log-util.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"


SRC_URI = "file://log-util.py \
           file://lib_pal.py \
          "

S = "${WORKDIR}"

DEPENDS += "libpal"

binfiles = "log-util.py lib_pal.py"

pkgdir = "log-util"

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  install -d $dst
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}

  install -m 755 log-util.py ${dst}/log-util
  ln -s ../fbpackages/${pkgdir}/log-util ${localbindir}/log-util

  install -m 755 lib_pal.py ${dst}/lib_pal.py
  ln -s ../fbpackages/${pkgdir}/lib_pal.py ${localbindir}/lib_pal.py
}

RDEPENDS_${PN} += "libpal"

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES_${PN} = "${FBPACKAGEDIR}/log-util ${prefix}/local/bin"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_PACKAGE_STRIP = "1"
