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
	"fmt"
	"os"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

func TestDisableHangPanic(t *testing.T) {
	// mock fileutils.WriteFileWithTimeout to return nil if the write is correct
	writeFileOrig := fileutils.WriteFileWithTimeout
	defer func() {
		fileutils.WriteFileWithTimeout = writeFileOrig
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
			writeFileErr: fmt.Errorf("WriteFile failed"),
			want: step.ExitSafeToReboot{
				Err: fmt.Errorf(
					"Failed to write to hang_task_panic file '/proc/sys/kernel/hung_task_panic': WriteFile failed",
				),
			},
		},
	}

	wantFilename := "/proc/sys/kernel/hung_task_panic"
	wantDataString := "0"

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.WriteFileWithTimeout = func(filename string, data []byte, perm os.FileMode, timeout time.Duration) error {
				if filename != wantFilename {
					return fmt.Errorf("filename: want %v got %v", wantFilename, filename)
				}
				if string(data) != wantDataString {
					return fmt.Errorf("data: want %v got %v", wantDataString, string(data))
				}
				return tc.writeFileErr
			}

			got := disableHangPanic(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
