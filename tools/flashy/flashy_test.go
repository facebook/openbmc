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
	"encoding/json"
	"fmt"
	"os"
	"path"
	"path/filepath"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
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

// getBuildNames reads build names from the list of build names in the
// repository
func getBuildNames() ([]string, error) {
	var buildNames []string
	buildNamesPath := filepath.Join(fileutils.SourceRootDir, "../platforms/platform_build_names")
	buildNamesBuf, err := fileutils.ReadFile(buildNamesPath)
	if err != nil {
		return buildNames, errors.Errorf("Failed to read '%v' for build names: %v", buildNamesPath, err)
	}
	err = json.Unmarshal(buildNamesBuf, &buildNames)
	if err != nil {
		return buildNames, errors.Errorf("Failed to read")
	}
	return buildNames, nil
}

// TestPlatformSpecificFiles tests to ensure that platform-specific files
// (e.g. flash_procedure/flash_wedge100.go) are well-formed.
func TestPlatformSpecificFiles(t *testing.T) {
	openBMCBuildNames, err := getBuildNames()
	if err != nil {
		t.Fatalf("Failed to get build names from repo: %v", err)
	}

	// buildNameCapturingGroup is a capturing group that captures build name
	// values and checks them against existing OpenBMC build names
	const buildNameCapturingGroup = "(?P<build_name>[^/]+)"

	// validPathPatternsMap maps from the subdirectory (relative to
	// fileutils.SourceRootDir) to valid patterns for non-`_test.go` .go
	// files allowed in that subdirectory
	validPathPatternsMap := map[string]([]string){
		"checks_and_remediations": []string{
			"common/(?P<step_name>[0-9]{2}_[^ ]+).go$",
			fmt.Sprintf("%v/(?P<step_name>[0-9]{2}_[^ ]+).go$", buildNameCapturingGroup),
		},
		"flash_procedure": []string{
			fmt.Sprintf("flash_%v.go$", buildNameCapturingGroup),
		},
	}

	for dir, patterns := range validPathPatternsMap {
		absDirPath := filepath.Join(fileutils.SourceRootDir, dir)
		err := filepath.Walk(absDirPath,
			func(path string, info os.FileInfo, err error) error {
				if err != nil {
					return err
				}

				if info.IsDir() || // skip directories
					(info.Mode()&os.ModeSymlink == os.ModeSymlink) || // skip symlinks to ignore alias system names.
					filepath.Ext(info.Name()) != ".go" || // skip non .go files
					tests.IsGoTestFileName(info.Name()) { // skip _test.go files
					return nil
				}

				// trim it to try to match the patterns in validPathPatternsMap
				subdirPath := path[len(dir)+1:]

				// try to match each pattern
				for _, pattern := range patterns {
					regExMap, err := utils.GetRegexSubexpMap(pattern, subdirPath)
					if err == nil {
						// if "build_name" is present in the map, check if it's
						// in the list of build names of existing OpenBMC platforms
						if buildName, ok := regExMap["build_name"]; ok {
							if utils.StringFind(buildName, openBMCBuildNames) == -1 {
								return errors.Errorf("'%v' file not allowed: %v is not an existing OpenBMC build name",
									path, buildName)
							}
						}
						return nil // test passed
					}
				}
				return errors.Errorf("'%v' file not allowed, does not match any of the given patterns: %#v",
					path, patterns)
			})
		if err != nil {
			t.Errorf("Test Platform-specific files failed: %v", err)
		}
	}
}
