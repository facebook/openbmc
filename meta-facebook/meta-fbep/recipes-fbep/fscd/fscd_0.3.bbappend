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

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://setup-fan.sh \
            file://fsc-config_AMD.json \
            file://fsc-config_NVIDIA.json \
            file://zone1_AMD.fsc \
            file://zone1_NVIDIA.fsc \
            file://fsc_board.py \
           "

FSC_CONFIG += "fsc-config_AMD.json \
               fsc-config_NVIDIA.json \
	      "

FSC_ZONE_CONFIG +="zone1_AMD.fsc \
                   zone1_NVIDIA.fsc \
                  "

FSC_INIT_FILE += "setup-fan.sh"
