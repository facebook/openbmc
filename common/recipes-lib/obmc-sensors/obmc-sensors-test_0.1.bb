# Copyright 2018-present Facebook. All Rights Reserved.
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

SUMMARY = "Test utility to check values of LM sensors"
DESCRIPTION = "Utility which can be used by developers to test out the chip/label names are functional"

SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://obmc-sensors-test.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

LOCAL_URI = " \
    file://Makefile.util \
    file://obmc-sensors-test.c \
    "

do_compile() {
  make -f Makefile.util
}

DEPENDS += "libobmc-sensors"
RDEPENDS:${PN} += "libobmc-sensors"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 obmc-sensors-test ${D}${bindir}/obmc-sensors-test
}

FILES:${PN} = "${bindir}/obmc-sensors-test"
