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
	"os"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/pkg/errors"
)

func TestDropCaches(t *testing.T) {
	// mock fileutils.WriteFile to return nil if the write is correct
	writeFileOrig := fileutils.WriteFile
	defer func() {
		fileutils.WriteFile = writeFileOrig
	}()

	cases := []struct {
		name         string
		writeFileErr error
		want         step.StepExitError
	}{
		{
			name:         "succeeded",
			writeFileErr: nil,
			want:         nil,
		},
		{
			name:         "WriteFile failed",
			writeFileErr: errors.Errorf("WriteFile failed"),
			want: step.ExitSafeToReboot{
				errors.Errorf(
					"Failed to write to drop_caches file '/proc/sys/vm/drop_caches': WriteFile failed",
				),
			},
		},
	}

	wantFilename := "/proc/sys/vm/drop_caches"
	wantDataString := "3"

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.WriteFile = func(filename string, data []byte, perm os.FileMode) error {
				if filename != wantFilename {
					return errors.Errorf("filename: want %v got %v", wantFilename, filename)
				}
				if string(data) != wantDataString {
					return errors.Errorf("data: want %v got %v", wantDataString, string(data))
				}
				return tc.writeFileErr
			}

			got := dropCaches(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
