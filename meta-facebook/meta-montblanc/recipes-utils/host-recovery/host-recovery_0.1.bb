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

SUMMARY = "OpenBMC Host-Recovery utilies"
SECTION = "base"
PR = "r0"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://cpld_update.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://cpld_update.sh \
    file://spi_util.sh \
"

HOSTRECOVERY_UTILS_FILES = " \
    cpld_update.sh \
    spi_util.sh \
"

do_install() {
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  for f in ${HOSTRECOVERY_UTILS_FILES}; do
      install -m 755 ${f} ${localbindir}/${f}
  done

}

RDEPENDS:${PN} += " \
    bash \
    udev-rules \
    jbi \
"

FILES:${PN} += "/usr/local/bin"
