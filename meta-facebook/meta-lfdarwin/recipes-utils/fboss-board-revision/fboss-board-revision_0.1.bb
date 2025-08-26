# Copyright 2025-present Facebook. All Rights Reserved.
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
SUMMARY = "FBOSS board revision"
DESCRIPTION = "Provides BMC type and revision name"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://fboss-board-revision.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

RDEPENDS:${PN} += " bash"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

LOCAL_URI = " \
    file://fboss-board-revision.sh \
    "

do_install() {
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}

  install -m 755 fboss-board-revision.sh ${localbindir}/fboss-board-revision.sh
}

FILES:${PN} = " ${sysconfdir} /usr/local"
