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

# Run this script to run each unit tests individually
set -euo pipefail

# Ensure we're in the project root
cd "$(dirname "$0")" && cd ..

echo "Running build test" >&2
if output=$(./build.sh); then
    echo "Build succeeded!"
else
    echo "Build failed!"
    echo "$output" >&2
    exit 1
fi

echo "Running unit tests" >&2
echo "Getting all packages..." >&2
all_packages=$(go list ./...)

echo "Getting list of tests..." >&2
for pkg in $all_packages
do
    pkg_tests=$(go test "${pkg}" -list .* | grep Test || true)
    for pkg_test in $pkg_tests
    do
        all_test_cmds+=("go test -v ${pkg} -run \"^${pkg_test}$\"")
    done
done

num_tests="${#all_test_cmds[@]}"
echo "Found $num_tests tests." >&2

echo "Starting to run tests" >&2

passed=0
failed=0
idx=1
for test_cmd in "${all_test_cmds[@]}"
do
    echo -n "[$idx/$num_tests] '$test_cmd': "

    if output=$(eval "$test_cmd" 2>&1); then
        echo "PASSED"
        ((++passed))
    else
        echo "FAILED"
        ((++failed))
        echo "$output" >&2
    fi

    ((++idx))
done

echo "$passed/$num_tests unit tests passed"


[[ $failed -ne 0 ]] && exit 1
exit 0
