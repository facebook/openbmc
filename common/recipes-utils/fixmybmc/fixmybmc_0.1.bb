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

SUMMARY = "FixMyBMC CLI Utility"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://fixmybmc/ \
    file://setup.py \
    file://fixmybmc.sh \
    "
S = "${WORKDIR}"

inherit setuptools3

PACKAGES = "${PN}"
FILES:${PN} += "${bindir}/* ${libdir}/python*/site-packages/*"

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/fixmybmc.sh ${D}${bindir}/fixmybmc
}


RDEPENDS:${PN} += "python3-core bash"
