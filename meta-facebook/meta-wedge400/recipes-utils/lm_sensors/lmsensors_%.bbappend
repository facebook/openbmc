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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://wedge400.conf \
            file://wedge400c.conf \
            file://wedge400-evt-mp.conf \
            file://wedge400-respin.conf \
            file://wedge400c-evt-dvt2.conf \
            file://wedge400c-respin.conf \
           "

DEPENDS:append = " update-rc.d-native"

do_install:append() {

    install -d ${D}${sysconfdir}/sensors.d
    install -d ${D}${sysconfdir}/sensors.d/custom
    install -m 644 ${WORKDIR}/wedge400.conf ${D}${sysconfdir}/sensors.d/custom/wedge400.conf
    install -m 644 ${WORKDIR}/wedge400c.conf ${D}${sysconfdir}/sensors.d/custom/wedge400c.conf
    install -m 644 ${WORKDIR}/wedge400-evt-mp.conf ${D}${sysconfdir}/sensors.d/custom/wedge400-evt-mp.conf
    install -m 644 ${WORKDIR}/wedge400-respin.conf ${D}${sysconfdir}/sensors.d/custom/wedge400-respin.conf
    install -m 644 ${WORKDIR}/wedge400c-evt-dvt2.conf ${D}${sysconfdir}/sensors.d/custom/wedge400c-evt-dvt2.conf
    install -m 644 ${WORKDIR}/wedge400c-respin.conf   ${D}${sysconfdir}/sensors.d/custom/wedge400c-respin.conf

}
