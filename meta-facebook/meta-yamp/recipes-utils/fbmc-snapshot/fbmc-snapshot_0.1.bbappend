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
    file://900_power_status.sh \
    file://901_versions.sh \
    file://902_weutil.sh \
    file://903_fan_info.sh \
    file://904_dump_cpld.sh \
    file://905_debug_logs.sh \
    "

SHOWTECH_RULES_FILES:append = " \
    900_power_status.sh \
    901_versions.sh \
    902_weutil.sh \
    903_fan_info.sh \
    904_dump_cpld.sh \
    905_debug_logs.sh \
    "
