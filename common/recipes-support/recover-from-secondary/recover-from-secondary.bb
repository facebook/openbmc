# Copyright 2014-present Facebook. All Rights Reserved.
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

#FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SUMMARY = "Reflash and recover devices that have booted from their second chip"
DESCRIPTION = "Restore from second chip"
SECTION = "base"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://adjust_bootargs.sh;beginline=4;endline=17;md5=0b1ee7d6f844d472fa306b2fee2167e0"

S = "${WORKDIR}"

SRC_URI = "\
        file://adjust_bootargs.sh \
        file://reflash_primary_chip.sh \
"

do_install() {
    bin="${D}${prefix}/local/bin"

    install -d $bin
    install -m 0755 adjust_bootargs.sh $bin
    install -m 0755 reflash_primary_chip.sh $bin
}

FILES_${PN} = "${prefix}/local/bin"
RDEPENDS_${PN} += "bash"
