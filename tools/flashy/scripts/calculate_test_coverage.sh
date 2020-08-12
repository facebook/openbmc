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

# Run this script to calculate test coverage in percentage
set -e

# Ensure we're in the project root
cd "$(dirname "$0")" && cd ..


# patterns of package paths to exclude
# NOTE: Please add a comment on why a package is excluded
excluded_packages=(
    # main package, no tests
    "github.com/facebook/openbmc/tools/flashy"
    # framework code (install)
    "github.com/facebook/openbmc/tools/flashy/install"
    # file-system side-effect code
    "github.com/facebook/openbmc/tools/flashy/lib/fileutils"
    # framework code (step)
    "github.com/facebook/openbmc/tools/flashy/lib/step"
    # test utilities
    "github.com/facebook/openbmc/tools/flashy/tests"
    # facebook-specific tests
    "github.com/facebook/openbmc/tools/flashy/tests/facebook"
    # framework code (logger)
    "github.com/facebook/openbmc/tools/flashy/lib/logger"
)

invert_filters=()
for i in "${excluded_packages[@]}"; do
    invert_filters+=( "-e \"^${i}$\"" )
done

mapfile -t included_packages < \
    <(eval "go list ./... | grep -v ${invert_filters[*]}")

tmp_dir=$(mktemp -d -t flashy-XXXXXXXXXXX)

go test ${included_packages[*]} -coverprofile "${tmp_dir}/cover.out" > /dev/null
go tool cover -func "${tmp_dir}/cover.out" | grep total | awk '{print substr($3, 1, length($3)-1)}'
rm -r "${tmp_dir}"
