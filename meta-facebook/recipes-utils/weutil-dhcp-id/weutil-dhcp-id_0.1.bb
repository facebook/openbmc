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
SUMMARY = "Scripts to output DHCP vendor information"
DESCRIPTION = "Scripts to output DHCP vendor information"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://dhcp-id;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

RDEPENDS:${PN} += " bash wedge-eeprom "

LOCAL_URI = " \
    file://dhcp-id \
    file://enterprise-num \
    "

do_install() {
  localbindir="${D}/usr/local/bin"
  install -d ${localbindir}
  install -m 755 dhcp-id ${localbindir}/dhcp-id
  install -m 755 enterprise-num ${localbindir}/enterprise-num
}

FILES:${PN} = " ${sysconfdir} /usr/local"
