# Copyright 2016-present Facebook. All Rights Reserved.
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

SUMMARY = "KV Store Library"
DESCRIPTION = "library for get set of kv pairs"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://kv.c;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

inherit meson python3-dir

SRC_URI = "\
    file://meson.build \
    file://kv \
    file://kv.c \
    file://kv.h \
    file://kv.py \
    "

S = "${WORKDIR}"

DEPENDS += "python3-setuptools"
RDEPENDS_${PN} += "python3-core bash"

do_install_append() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    install -m 644 ${S}/kv.py ${D}${PYTHON_SITEPACKAGES_DIR}/
}
FILES_${PN} += "${PYTHON_SITEPACKAGES_DIR}/kv.py"

BBCLASSEXTEND = "native nativesdk"
