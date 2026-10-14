# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

LOCAL_URI += "\
    file://900_sensors.sh \
    "

SHOWTECH_RULES_FILES:append = " \
    900_sensors.sh \
    "

# Montblanc systems running Netlake 2.0 lack APML support for 
# standard sensor data collection. The script `004_sensors.sh` 
# has been replaced with `900_sensors.sh` to enable dynamic 
# adaptation for both Netlake 1.0 and 2.0.
SHOWTECH_RULES_FILES:remove = "004_sensors.sh"
