# Copyright 2023-present Facebook. All Rights Reserved.
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
    file://fsc-config-gta-8-retimer.json \
    file://zone-fsc-gta-8-retimer.fsc \
    file://fsc-config-gta-8-retimer-dis-vpdb.json \
    file://zone-fsc-gta-8-retimer-dis-vpdb.fsc \
    file://fsc-config-gta-8-retimer-freya.json \
    file://zone-fsc-gta-8-retimer-freya.fsc \
    "

FSC_CONFIG += "fsc-config-gta-8-retimer.json \
               fsc-config-gta-8-retimer-dis-vpdb.json \
               fsc-config-gta-8-retimer-freya.json \
	          "

FSC_ZONE_CONFIG +="zone-fsc-gta-8-retimer.fsc \
                   zone-fsc-gta-8-retimer-dis-vpdb.fsc \
                   zone-fsc-gta-8-retimer-freya.fsc \
	              "

RDEPENDS:${PN} += "bash"
