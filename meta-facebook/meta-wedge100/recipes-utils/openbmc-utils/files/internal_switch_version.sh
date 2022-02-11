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
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
set -e
set -o pipefail
exit_code=2  

. /usr/local/bin/openbmc-utils.sh

EEPROM_FILE="/tmp/.tmp_oob_eeprom.bin"

cleanup () {
  rm -f "$EEPROM_FILE"
  exit $exit_code
}
trap cleanup EXIT ERR INT TERM

if ! out=$(/usr/local/bin/spi_util.sh read spi2 OOB_SWITCH_EEPROM "$EEPROM_FILE" > /dev/null 2>&1); then
    echo "Error: failed to read the switch eeprom!"
    echo "$out"
    exit 1
fi

sha256sum < "$EEPROM_FILE" | head -c 64
exit_code=$?
