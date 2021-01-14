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

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://setup-fan.sh \
            file://setup-fsc-config.sh \
            file://FSC_GC_Type5_EVT_v1_zone0.fsc \
            file://FSC_GC_Type5_EVT_v1_config.json \
            file://FSC_GC_Type7_EVT_v1_zone0.fsc \
            file://FSC_GC_Type7_EVT_v1_config.json \
            file://fsc_board.py \
           "
FSC_CONFIG += "FSC_GC_Type5_EVT_v1_config.json \
               FSC_GC_Type7_EVT_v1_config.json \
              "

FSC_ZONE_CONFIG += "FSC_GC_Type5_EVT_v1_zone0.fsc \
                    FSC_GC_Type7_EVT_v1_zone0.fsc \
                   "

FSC_BIN_FILES += "setup-fsc-config.sh"

FSC_INIT_FILE += "setup-fan.sh"
