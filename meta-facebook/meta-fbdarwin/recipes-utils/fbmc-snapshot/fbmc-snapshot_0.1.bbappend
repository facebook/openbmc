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
    file://oob-eeprom-util.sh \
    "

SHOWTECH_UTILS_FILES:append = " oob-eeprom-util.sh"

# The OOB switch here is a BCM53134P, so openbmc-utils owns oob-mdio-util.sh.
# files/oob-status.sh overrides the layer copy to match it; the two are a pair
# and must be changed together.
SHOWTECH_UTILS_FILES:remove = "oob-mdio-util.sh"
