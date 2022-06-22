# Copyright 2019-present Facebook. All Rights Reserved.
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

LOCAL_URI += " \
    file://rest_feutil.py \
    file://rest_firmware_info.py \
    file://rest_gb_freq.py \
    file://rest_presence.py \
    file://test_rest_presence.py \
    file://rest_seutil.py \
    file://rest_sensors.py \
    file://rest_switch_reset.py \
    file://rest_vddcore.py \
    file://acl_providers/wedge400_acl_provider.py \
    file://acl_config.py \
    file://run_rest \
"

aclfiles += "wedge400_acl_provider.py"

binfiles += "acl_config.py"

binfiles1 += " \
    rest_feutil.py \
    rest_firmware_info.py \
    rest_gb_freq.py \
    rest_presence.py \
    rest_sensors.py \
    rest_seutil.py \
    rest_switch_reset.py \
    rest_vddcore.py \
"
