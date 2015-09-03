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

SUMMARY = "OpenBMC Utilities"
DESCRIPTION = "Various OpenBMC utilities"
SECTION = "base"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=eb723b61539feef013de476e68b5c50a"

SRC_URI = " \
    file://COPYING \
    file://openbmc-utils.sh \
    "

OPENBMC_UTILS_FILES = " \
    openbmc-utils.sh \
    "

S = "${WORKDIR}"

do_install() {
    pkgdir="/usr/local/packages/utils"
    dstdir="${D}${pkgdir}"
    install -d $dstdir
    localbindir="${D}/usr/local/bin"
    install -d ${localbindir}
    for f in ${OPENBMC_UTILS_FILES}; do
        install -m 755 $f ${dstdir}/${f}
        ln -s ${pkgdir}/${f} ${localbindir}
    done
}

FILES_${PN} += "/usr/local"
