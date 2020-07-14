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

package common

import (
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

type ResolveFilePatternsReturnType struct {
	files *[]string
	err   error
}

func TestTruncateLogs(t *testing.T) {
	// save and defer restore TruncateFile, RemoveFile and resolveFilePatterns
	truncateFileOrig := utils.TruncateFile
	removeFileOrig := utils.RemoveFile
	resolveFilePatternsOrig := resolveFilePatterns

	defer func() {
		utils.TruncateFile = truncateFileOrig
		utils.RemoveFile = removeFileOrig
		resolveFilePatterns = resolveFilePatternsOrig
	}()

	cases := []struct {
		name string
		// resolveFilePatterns is called twice
		// return the first one when called the first time,
		// and so on
		resolvedFilePatterns []ResolveFilePatternsReturnType
		removeFileErr        error
		truncateFileErr      error
		want                 utils.StepExitError
	}{
		{
			name: "Normal operation",
			resolvedFilePatterns: []ResolveFilePatternsReturnType{
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            nil,
		},
		{
			name: "Remove file error",
			resolvedFilePatterns: []ResolveFilePatternsReturnType{
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   errors.Errorf("RemoveFile Error"),
			truncateFileErr: nil,
			want:            utils.ExitSafeToReboot{errors.Errorf("Unable to remove log file '/tmp/test': RemoveFile Error")},
		},
		{
			name: "Truncate file error",
			resolvedFilePatterns: []ResolveFilePatternsReturnType{
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: errors.Errorf("TruncateFile Error"),
			want:            utils.ExitSafeToReboot{errors.Errorf("Unable to truncate log file '/tmp/test': TruncateFile Error")},
		},
		{
			name: "Resolve file patterns error",
			resolvedFilePatterns: []ResolveFilePatternsReturnType{
				ResolveFilePatternsReturnType{
					nil,
					errors.Errorf("resolveFilePatterns Error"),
				},
				ResolveFilePatternsReturnType{
					&[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            utils.ExitSafeToReboot{errors.Errorf("Unable to resolve file patterns '%v': resolveFilePatterns Error", deleteLogFilePatterns)},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			resolveFilePatternsCalled := 0
			resolveFilePatterns = func(patterns []string) (*[]string, error) {
				ret := tc.resolvedFilePatterns[resolveFilePatternsCalled]
				resolveFilePatternsCalled++
				return ret.files, ret.err
			}
			utils.TruncateFile = func(filename string, size int64) error {
				return tc.truncateFileErr
			}
			utils.RemoveFile = func(filename string) error {
				return tc.removeFileErr
			}
			got := truncateLogs("x", "x")
			tests.CompareTestExitErrors(tc.want, got, t)
		})
	}
}

func TestResolveFilePatterns(t *testing.T) {
	cases := []struct {
		name     string
		patterns []string
		// these tests should be system agnostic so even if the
		// path is valid we cannot test for how many files we got
		// hence, we can only test for the error
		want error
	}{
		{
			name:     "Empty pattern",
			patterns: []string{},
			want:     nil,
		},
		{
			name:     "Test multiple valid patterns",
			patterns: []string{"/var/*", "/var/log/messages"},
			want:     nil,
		},
		{
			name:     "Invalid pattern",
			patterns: []string{"["},
			want:     errors.Errorf("Unable to resolve pattern '[': syntax error in pattern"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			_, got := resolveFilePatterns(tc.patterns)

			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
