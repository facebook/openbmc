#!/bin/bash
# Copyright 2020-present Facebook. All Rights Reserved.
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

# Run this script to get a list of all test commands for each test function
set -e

# Ensure we're in the project root
cd "$(dirname "$0")" && cd ..

all_packages=$(go list ./...)

for pkg in $all_packages
do
    pkg_tests=$(go test "${pkg}" -list .* | grep Test || true)
    for pkg_test in $pkg_tests
    do
        all_test_cmds+=("go test -v ${pkg} -run \"^${pkg_test}$\"")
    done
done

printf '%s\n' "${all_test_cmds[@]}"

# uncomment the below to run the tests
# for test_cmd in "${all_test_cmds[@]}"
# do
#     $test_cmd
# done
