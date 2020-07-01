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
#
SUMMARY = "Bridge IC Utility"
DESCRIPTION = "Util for checking with Bridge IC"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://bic-util.c;beginline=4;endline=16;md5=b395943ba8a0717a83e62ca123a8d238"

SRC_URI = "file://bic-util.c \
           file://Makefile \
          "

S = "${WORKDIR}"

CFLAGS += "-Wall -Werror"
LDFLAGS += "-lbic -lpal -lfruid"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 bic-util ${D}${bindir}/bic-util
}
DEPENDS += "libbic libpal libfruid"
RDEPENDS_${PN} += "libbic libpal libfruid"

FILES_${PN} = "${bindir}"
