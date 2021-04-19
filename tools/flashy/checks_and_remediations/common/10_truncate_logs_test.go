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
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type GlobAllReturnType struct {
	files []string
	err   error
}

func TestTruncateLogs(t *testing.T) {
	// save and defer restore TruncateFile, RemoveFile and resolveFilePatterns
	truncateFileOrig := fileutils.TruncateFile
	removeFileOrig := fileutils.RemoveFile
	globAllOrig := fileutils.GlobAll
	runCommandOrig := utils.RunCommand

	defer func() {
		fileutils.TruncateFile = truncateFileOrig
		fileutils.RemoveFile = removeFileOrig
		fileutils.GlobAll = globAllOrig
		utils.RunCommand = runCommandOrig
	}()

	cases := []struct {
		name string
		// resolveFilePatterns is called twice
		// return the first one when called the first time,
		// and so on
		resolvedFilePatterns []GlobAllReturnType
		removeFileErr        error
		truncateFileErr      error
		want                 step.StepExitError
	}{
		{
			name: "Normal operation",
			resolvedFilePatterns: []GlobAllReturnType{
				{
					[]string{"/tmp/test"},
					nil,
				},
				{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            nil,
		},
		{
			name: "Resolve file patterns error (1)",
			resolvedFilePatterns: []GlobAllReturnType{
				{
					nil,
					errors.Errorf("resolveFilePatterns Error"),
				},
				{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            step.ExitSafeToReboot{Err: errors.Errorf("Unable to resolve file patterns '%v': resolveFilePatterns Error", deleteLogFilePatterns)},
		},
		{
			name: "Resolve file patterns error (2)",
			resolvedFilePatterns: []GlobAllReturnType{
				{
					[]string{"/tmp/test"},
					nil,
				},
				{
					nil,
					errors.Errorf("resolveFilePatterns Error"),
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            step.ExitSafeToReboot{Err: errors.Errorf("Unable to resolve file patterns '%v': resolveFilePatterns Error", deleteLogFilePatterns)},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			resolveFilePatternsCalled := 0
			journalctlRan := false
			fileutils.GlobAll = func(patterns []string) ([]string, error) {
				ret := tc.resolvedFilePatterns[resolveFilePatternsCalled]
				resolveFilePatternsCalled++
				return ret.files, ret.err
			}
			fileutils.TruncateFile = func(filename string, size int64) error {
				return tc.truncateFileErr
			}
			fileutils.RemoveFile = func(filename string) error {
				return tc.removeFileErr
			}
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				journalctlRan = (cmdArr[0] == "journalctl")
				return 0, nil, "", ""
			}
			got := truncateLogs(step.StepParams{})
			if !journalctlRan {
				t.Errorf("journalctl not executed")
			}
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
