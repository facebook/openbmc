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
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
	"github.com/pkg/errors"
)

func TestValidateImagePartitions(t *testing.T) {
	validateImageFileOrig := image.ValidateImageFile
	isPfrSystemOrig := utils.IsPfrSystem
	defer func() {
		image.ValidateImageFile = validateImageFileOrig
		utils.IsPfrSystem = isPfrSystemOrig
	}()

	mockDeviceID := "mtd:flash0"

	cases := []struct {
		name          string
		clowntown     bool
		isPfrSystem   bool
		validateError error
		want          step.StepExitError
	}{
		{
			name:          "successful validation",
			clowntown:     false,
			isPfrSystem:   false,
			validateError: nil,
			want:          nil,
		},
		{
			name:          "PFR system, bypass",
			clowntown:     false,
			isPfrSystem:   true,
			validateError: nil,
			want:          nil,
		},
		{
			name:          "validation failed",
			clowntown:     false,
			isPfrSystem:   false,
			validateError: errors.Errorf("validation failed"),
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("validation failed"),
			},
		},
		{
			name:          "clowntown bypass",
			clowntown:     true,
			isPfrSystem:   false,
			validateError: errors.Errorf("validation failed"),
			want:          nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			imageFileName := "/run/upgrade/image"
			utils.IsPfrSystem = func() bool {
				return tc.isPfrSystem
			}
			image.ValidateImageFile = func(imageFilePath string, deviceID string) error {
				if imageFilePath != imageFileName {
					t.Errorf("Image filename: want '%v' got '%v'",
						imageFileName, imageFilePath)
				}
				if deviceID != mockDeviceID {
					t.Errorf("deviceID: want '%v' got '%v'", mockDeviceID, deviceID)
				}
				return tc.validateError
			}
			stepParams := step.StepParams{
				Clowntown:     tc.clowntown,
				ImageFilePath: imageFileName,
				DeviceID:      mockDeviceID,
			}

			got := validateImagePartitions(stepParams)

			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
