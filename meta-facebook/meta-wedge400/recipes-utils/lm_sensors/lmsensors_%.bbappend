#
# Copyright 2019-present Facebook. All Rights Reserved.
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

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://wedge400.conf \
            file://wedge400c.conf \
           "

DEPENDS_append = " update-rc.d-native"

do_install_append() {

    install -d ${D}${sysconfdir}/sensors.d
    install -d ${D}${sysconfdir}/sensors.d/custom
    install -m 644 ${WORKDIR}/wedge400.conf ${D}${sysconfdir}/sensors.d/custom/wedge400.conf
    install -m 644 ${WORKDIR}/wedge400c.conf ${D}${sysconfdir}/sensors.d/custom/wedge400c.conf

}
