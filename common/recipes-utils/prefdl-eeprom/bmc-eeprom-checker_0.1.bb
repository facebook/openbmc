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

SUMMARY = "bmc-eeprom-checker"
DESCRIPTION = "Check the BMC's EEPROM format"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://bmc_eeprom_checker.sh;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://bmc_eeprom_checker.sh \
    "

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 ${UNPACKDIR}/bmc_eeprom_checker.sh ${D}/usr/bin
}

RDEPENDS:${PN} += "bash"
FILES:${PN} += "${prefix}/bin ${sysconfdir} "
