# Copyright 2017-present Facebook. All Rights Reserved.
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
SUMMARY = "NVMe-MI library"
DESCRIPTION = "library for NVMe-MI"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://nvme-mi.c;beginline=4;endline=16;md5=7783b537a8ff52cf362d3cdb4bb0f6e2"

SRC_URI = "file://src \
          "

LDFLAGS += "-lobmc-i2c"
DEPENDS += "liblog libobmc-i2c"

S = "${WORKDIR}/src"

do_install() {
    install -d ${D}${libdir}
    install -m 0644 libnvme-mi.so ${D}${libdir}/libnvme-mi.so

    install -d ${D}${includedir}/openbmc
    install -m 0644 nvme-mi.h ${D}${includedir}/openbmc/nvme-mi.h
}

FILES_${PN} = "${libdir}/libnvme-mi.so"
FILES_${PN}-dev = "${includedir}/openbmc/nvme-mi.h"

DEPENDS += " liblog libobmc-i2c "
