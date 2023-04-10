# Copyright 2023-present Mate Platform. All Rights Reserved.
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

SUMMARY = "Wedge EEPROM Utilities"
DESCRIPTION = "Util for BMC-lite wedge eeprom"
SECTION = "base"
PR = "r0"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Weutil.cpp;beginline=1;endline=12;md5=2c1289c9e6131aff18d3dcec3698c7c0"

inherit meson pkgconfig
inherit python3native
inherit python3-dir

CFLAGS += "-Wall -Werror"

SRC_URI = "file://utils  \
           file://lib/WeutilInterface.h "


S = "${WORKDIR}/utils"

do_install() {
	  install -d ${D}${bindir}
    install -m 0755 weutil ${D}${bindir}/weutil
}

DEPENDS += "libweutil"
RDEPENDS:${PN} = "libweutil"
FILES:${PN} = "${bindir}"

