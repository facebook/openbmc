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
            file://FSC_CLASS2_EVT_config.json \
            file://FSC_CLASS2_EVT_zone1.fsc \
            file://FSC_CLASS1_EVT_type1.json \
            file://FSC_CLASS1_EVT_type10.json \
            file://FSC_CLASS1_EVT_type10_zone1.fsc \
            file://FSC_CLASS1_EVT_type1_zone1.fsc \
           "
FSC_CONFIG += "FSC_CLASS1_EVT_type1.json \
               FSC_CLASS1_EVT_type10.json \
               FSC_CLASS2_EVT_config.json \
              "

FSC_ZONE_CONFIG += " FSC_CLASS2_EVT_zone1.fsc \
                     FSC_CLASS1_EVT_type10_zone1.fsc \
                     FSC_CLASS1_EVT_type1_zone1.fsc \
                   "
FSC_INIT_FILE += "setup-fan.sh"
