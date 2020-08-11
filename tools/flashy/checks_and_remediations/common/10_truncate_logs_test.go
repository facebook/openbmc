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

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
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

	defer func() {
		fileutils.TruncateFile = truncateFileOrig
		fileutils.RemoveFile = removeFileOrig
		fileutils.GlobAll = globAllOrig
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
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            nil,
		},
		{
			name: "Remove file error",
			resolvedFilePatterns: []GlobAllReturnType{
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   errors.Errorf("RemoveFile Error"),
			truncateFileErr: nil,
			want:            step.ExitSafeToReboot{errors.Errorf("Unable to remove log file '/tmp/test': RemoveFile Error")},
		},
		{
			name: "Truncate file error",
			resolvedFilePatterns: []GlobAllReturnType{
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: errors.Errorf("TruncateFile Error"),
			want:            step.ExitSafeToReboot{errors.Errorf("Unable to truncate log file '/tmp/test': TruncateFile Error")},
		},
		{
			name: "Resolve file patterns error (1)",
			resolvedFilePatterns: []GlobAllReturnType{
				GlobAllReturnType{
					nil,
					errors.Errorf("resolveFilePatterns Error"),
				},
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            step.ExitSafeToReboot{errors.Errorf("Unable to resolve file patterns '%v': resolveFilePatterns Error", deleteLogFilePatterns)},
		},
		{
			name: "Resolve file patterns error (2)",
			resolvedFilePatterns: []GlobAllReturnType{
				GlobAllReturnType{
					[]string{"/tmp/test"},
					nil,
				},
				GlobAllReturnType{
					nil,
					errors.Errorf("resolveFilePatterns Error"),
				},
			},
			removeFileErr:   nil,
			truncateFileErr: nil,
			want:            step.ExitSafeToReboot{errors.Errorf("Unable to resolve file patterns '%v': resolveFilePatterns Error", deleteLogFilePatterns)},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			resolveFilePatternsCalled := 0
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
			got := truncateLogs(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
