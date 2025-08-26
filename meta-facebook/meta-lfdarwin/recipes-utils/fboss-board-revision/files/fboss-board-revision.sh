#!/bin/bash
#
# Copyright 2025-present Facebook. All Rights Reserved.
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

if [ -z "$1" ]; then
  set -- --help
fi

while [ "$1" ]; do
  case "$1" in
    --help)
      echo "Prints board revision information."
      echo "Usage: fboss-board-revision.sh [-r] [-s]"
      echo "Multiple options can be given, each will print the result in order"
      echo "Options:"
      echo "  -r: Prints 1 if respin, 0 if not, 'unknown' if unknown"
      echo "  -s: Prints board type/revision as a human-readable string"
      ;;
    -r)
      # Assume P1
        echo 1
      ;;
    -s)
      echo 'FBDARWIN'
      ;;
    *)
      echo "UNKNOWN OPTION"
      ;;
  esac
  shift
done

exit 0
