#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

inherit systemd

SUMMARY = "Console auto discovery"
DESCRIPTION = "Print special string to the serial console"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://console_autodiscovery.py;beginline=5;endline=18;md5=0b1ee7d6f844d472fa306b2fee2167e0"

LOCAL_URI = " \
    file://console_autodiscovery.py \
    file://console_autodiscovery.service \
"

do_install() {
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${S}/console_autodiscovery.py ${D}/usr/local/bin
    install -m 0644 ${S}/console_autodiscovery.service ${D}${systemd_system_unitdir}
}

RDEPENDS:${PN} += "python3-core "
FILES:${PN} = "/usr/local/bin ${sysconfdir} "
SYSTEMD_SERVICE:${PN} = "console_autodiscovery.service"