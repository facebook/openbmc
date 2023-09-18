#!/bin/sh
#
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
#

KV_CMD="/usr/bin/kv"
default_fsc_config="/etc/fsc-config.json"

$KV_CMD set "auto_fsc_config" "gtartemis" persistent

if [[ -f ${default_fsc_config} ]]; then
  rm ${default_fsc_config}
fi

ln -s /etc/fsc-config-gta-8-retimer.json ${default_fsc_config}

/etc/init.d/check-fan-cache.sh &
