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

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

LOCAL_URI += " \
    file://setup-fan.sh \
    file://fsc-config-evt.json \
    file://fsc-config-2-retimer.json \
    file://fsc-config-8-retimer.json \
    file://zone-evt.fsc \
    file://zone-2-retimer.fsc \
    file://zone-8-retimer.fsc \
    "

FSC_CONFIG += "fsc-config-evt.json \
               fsc-config-2-retimer.json \
               fsc-config-8-retimer.json \
	      "

FSC_ZONE_CONFIG +="zone-evt.fsc \
                   zone-2-retimer.fsc \
                   zone-8-retimer.fsc \
	          "
RDEPENDS:${PN} += "bash"
