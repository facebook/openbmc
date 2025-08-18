#!/bin/bash
#
# Copyright 2024-present Facebook. All Rights Reserved.
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

# This script makes weutil backward-compatible on DARWIN platforms, where
# DARWIN48V uses Meta EEPROM V5 and the new parser, but DARWIN uses the
# older prefdl EEPROM format and the old parser.

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

# If weutil is invoked in the old prefdl parser style, meaning the EEPROM
# target [chassis|bmc] is passed directly to the utility without the
# [-e|--eeprom] flag, then pass that target to the prefdl parser which
# supports both platforms. Otherwise, check if the BMC EEPROM contains
# Meta EEPROM V5 content at the expected 15K offset: if yes, then pass
# the arguments to the new parser; if no, pass the arguments to the old
# parser.
prefdl_util_arg=""
e_flag_arg=1
all_args=("$@")
while test $# -gt 0; do
    case "${1,,}" in
       -e|--eeprom)
            e_flag_arg=0
            shift
            if test $# -gt 0; then
                shift
            fi
            ;;
       bmc|chassis)
            prefdl_util_arg="$1"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

if [ -n "$prefdl_util_arg" ]; then
    /usr/bin/weutil_prefdl "${all_args[@]}"
elif bmc_has_meta_eeprom; then
    /usr/bin/weutil_meta "${all_args[@]}"
elif [ $e_flag_arg -eq 1 ]; then
    /usr/bin/weutil_prefdl "${all_args[@]}"
fi
