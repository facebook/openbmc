#
# Copyright 2020-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Add custom ssifd.service override
LOCAL_URI += " \
    file://ssifd.service \
    "

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/ssifd.service ${D}${systemd_system_unitdir}/ssifd.service
  
    rm -f ${D}${prefix}/local/bin/check_ssifd.sh
}

FILES:${PN} += "${systemd_system_unitdir}/ssifd.service"

# Remove SYSTEMD_SERVICE from the main recipe to prevent it from being automatically enabled.
SYSTEMD_SERVICE:${PN} = ""