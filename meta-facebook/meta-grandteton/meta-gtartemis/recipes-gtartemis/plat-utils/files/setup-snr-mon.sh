#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

#MB_HSC_MODULE="1"  # ltc4282/ltc4286
if kv get mb_hsc_module >/dev/null; then
  sed -i "2{s/$/ hsc/}" /etc/sv/sensord/run
fi

kv set mb_polling_status      1
kv set nic0_polling_status    1
kv set nic1_polling_status    1
kv set scm_polling_status     1
kv set vpdb_polling_status    1
kv set hpdb_polling_status    1
kv set fan_bp1_polling_status 1
kv set fan_bp2_polling_status 1
