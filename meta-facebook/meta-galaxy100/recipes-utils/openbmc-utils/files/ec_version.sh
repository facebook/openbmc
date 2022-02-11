#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

EC_VERSION=$(head -n 1 "${GALAXY_EC_SYSFS_DIR}/version")

r=$(echo "$EC_VERSION" | awk '{print substr ($1, 2, 2)}')
e=$(echo "$EC_VERSION" | awk '{print substr ($1, 5, 2)}')

released_date=$(head -n 1 "${GALAXY_EC_SYSFS_DIR}/build_date_input")

echo "EC Released Version: R$(printf '%02d' $r).E$(printf '%02d' $e)"
echo "EC Released Date: $released_date"
