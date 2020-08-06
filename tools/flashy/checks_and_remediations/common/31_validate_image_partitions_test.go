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
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/pkg/errors"
)

func TestValidateImagePartitions(t *testing.T) {
	// mock and defer restore MmapFile and
	// ValidateImage
	validateImageOrig := validate.ValidateImage
	mmapFileOrig := fileutils.MmapFile
	defer func() {
		validate.ValidateImage = validateImageOrig
		fileutils.MmapFile = mmapFileOrig
	}()

	cases := []struct {
		name                    string
		clowntown               bool
		mmapErr                 error
		validatePartitionsError error
		want                    step.StepExitError
	}{
		{
			name:                    "successful validation",
			clowntown:               false,
			mmapErr:                 nil,
			validatePartitionsError: nil,
			want:                    nil,
		},
		{
			name:                    "mmap error",
			clowntown:               false,
			mmapErr:                 errors.Errorf("mmap failed"),
			validatePartitionsError: nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Unable to read image file '%v': %v",
					"/opt/upgrade/image", errors.Errorf("mmap failed")),
			},
		},
		{
			name:                    "validation failed",
			clowntown:               false,
			mmapErr:                 nil,
			validatePartitionsError: errors.Errorf("validation failed"),
			want: step.ExitUnknownError{
				errors.Errorf("validation failed"),
			},
		},
		{
			name:                    "clowntown bypass",
			clowntown:               true,
			mmapErr:                 nil,
			validatePartitionsError: errors.Errorf("validation failed"),
			want:                    nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			validate.ValidateImage = func(data []byte) error {
				return tc.validatePartitionsError
			}
			fileutils.MmapFile = func(filename string, prot, flags int) ([]byte, error) {
				return nil, tc.mmapErr
			}

			stepParams := step.StepParams{
				Clowntown:     tc.clowntown,
				ImageFilePath: "/opt/upgrade/image",
			}

			got := ValidateImagePartitions(stepParams)

			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
