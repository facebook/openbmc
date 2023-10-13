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

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

RDEPENDS:${PN} += "bash"

LOCAL_URI += " \
    file://get_fan_speed.sh \
    file://set_fan_speed.sh \
    file://fsc_board.py \
    file://fsc-config-rtsw.json \
    file://fsc-config-non-rtsw.json \
    file://zone.fsc \
    file://setup-fan.sh \
    "

FSC_BIN_FILES += "get_fan_speed.sh \
                  set_fan_speed.sh "

RDEPENDS:${PN} += "bash"
FSC_CONFIG += "fsc-config-rtsw.json"
FSC_CONFIG += "fsc-config-non-rtsw.json"

FSC_ZONE_CONFIG +="zone.fsc"

FSC_INIT_FILE += "setup-fan.sh"
