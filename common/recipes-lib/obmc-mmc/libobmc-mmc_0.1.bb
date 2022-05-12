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

SUMMARY = "eMMC Device Access Library"
DESCRIPTION = "Library for accessing eMMC devices"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://obmc-mmc.h;beginline=7;endline=19;md5=da35978751a9d71b73679307c4d296ec"

BBCLASSEXTEND = "native"

inherit meson pkgconfig

LOCAL_URI = " \
    file://lsmmc.c \
    file://mmc.h \
    file://mmc_int.h \
    file://obmc-mmc.c \
    file://obmc-mmc.h \
    file://meson.build \
    file://obmc_mmc.py \
    "

inherit python3-dir

do_install:append() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/obmc_mmc.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}
FILES:${PN} += "${PYTHON_SITEPACKAGES_DIR}/obmc_mmc.py"
