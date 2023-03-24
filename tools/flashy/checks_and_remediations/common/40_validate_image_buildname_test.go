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

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/pkg/errors"
)

func TestValidateImageBuildname(t *testing.T) {
	// mock and defer restore CheckImageBuildNameCompatibility
	checkImageBuildNameCompatibilityOrig := validate.CheckImageBuildNameCompatibility
	defer func() {
		validate.CheckImageBuildNameCompatibility = checkImageBuildNameCompatibilityOrig
	}()
	cases := []struct {
		name             string
		clowntown        bool
		compatibilityErr error
		want             step.StepExitError
	}{
		{
			name:             "passed",
			clowntown:        false,
			compatibilityErr: nil,
			want:             nil,
		},
		{
			name:             "compatibility failed",
			clowntown:        false,
			compatibilityErr: errors.Errorf("compatibility failed"),
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Image build name compatibility check failed: " +
					"compatibility failed. Use the '--clowntown' flag if you wish " +
					"to proceed at the risk of bricking the device"),
			},
		},
		{
			name:             "bypassed using clowntown",
			clowntown:        true,
			compatibilityErr: errors.Errorf("compatibility failed"),
			want:             nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleImageFilePath := "/run/upgrade/mock"
			validate.CheckImageBuildNameCompatibility = func(imageFilePath string) error {
				if exampleImageFilePath != imageFilePath {
					t.Errorf("imageFilePath: want '%v' got '%v'", exampleImageFilePath, imageFilePath)
				}
				return tc.compatibilityErr
			}
			got := validateImageBuildname(step.StepParams{
				Clowntown:     tc.clowntown,
				ImageFilePath: exampleImageFilePath,
			})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
