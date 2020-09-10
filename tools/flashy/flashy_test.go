/**
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

package main

import (
	"path"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

// make sure that test coverage passes minimum coverage required
const minimumCoverage = 90.0

var absTestCovScriptPath = path.Join(
	fileutils.SourceRootDir,
	"scripts",
	"calculate_test_coverage.sh",
)

func TestCoverage(t *testing.T) {
	_, err, stdout, _ := utils.RunCommand([]string{absTestCovScriptPath}, 30*time.Second)
	if err != nil {
		t.Errorf("Couldn't calculate test coverage due to: %v. Check other failing tests.", err)
		return
	}
	pctStr := strings.TrimSpace(stdout)
	pct, err := strconv.ParseFloat(pctStr, 64)
	if err != nil {
		t.Errorf("Couldn't parse test coverage script output: %v", err)
		return
	}
	if pct < minimumCoverage {
		t.Errorf("Current test coverage (%v%%) lower than required test coverage (%v%%)",
			pct, minimumCoverage)
	}
}
