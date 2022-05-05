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
    file://fsc-config_AMD_vicor.json \
    file://fsc-config_AMD_infineon.json \
    file://fsc-config_NVIDIA_vicor.json \
    file://fsc-config_NVIDIA_infineon.json \
    file://zone1_AMD_vicor.fsc \
    file://zone1_AMD_infineon.fsc \
    file://zone1_NVIDIA_vicor.fsc \
    file://zone1_NVIDIA_infineon.fsc \
    file://fsc_board.py \
    "

FSC_CONFIG += "fsc-config_AMD_vicor.json \
               fsc-config_AMD_infineon.json \
               fsc-config_NVIDIA_vicor.json \
               fsc-config_NVIDIA_infineon.json \
	      "

FSC_ZONE_CONFIG +="zone1_AMD_vicor.fsc \
                   zone1_AMD_infineon.fsc \
                   zone1_NVIDIA_vicor.fsc \
                   zone1_NVIDIA_infineon.fsc \
                  "

FSC_INIT_FILE += "setup-fan.sh"
