/**
 * Copyright 2021-present Facebook. All Rights Reserved.
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

package remediations_grandcanyon

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/pkg/errors"
)

func TestValidateImageMetaPartitions(t *testing.T) {
	validateImageFileOrig := image.ValidateImageFile
	defer func() {
		image.ValidateImageFile = validateImageFileOrig
	}()

	cases := []struct {
		name          string
		clowntown     bool
		validateError error
		want          step.StepExitError
	}{
		{
			name:          "successful validation",
			clowntown:     false,
			validateError: nil,
			want:          nil,
		},
		{
			name:          "validation failed",
			clowntown:     false,
			validateError: errors.Errorf("validation failed"),
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("validation failed"),
			},
		},
		{
			name:          "clowntown bypass",
			clowntown:     true,
			validateError: errors.Errorf("validation failed"),
			want:          nil,
		},
	}
	const mockDeviceID = "mtd:flashmock"
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			imageFileName := "/opt/upgrade/image"
			image.ValidateImageFile = func(imageFilePath string, deviceID string, imageFormats []partition.ImageFormat) error {
				if imageFilePath != imageFileName {
					t.Errorf("Image filename: want '%v' got '%v'",
						imageFileName, imageFilePath)
				}
				if deviceID != mockDeviceID {
					t.Errorf("deviceID: want '%v' got '%v'", mockDeviceID, deviceID)
				}
				if !reflect.DeepEqual(imageFormats, []partition.ImageFormat{partition.MetaPartitionImageFormat}) {
					t.Errorf("imageFormats: want '%v' got '%v'",
						[]partition.ImageFormat{partition.MetaPartitionImageFormat},
						imageFormats)
				}
				return tc.validateError
			}
			stepParams := step.StepParams{
				Clowntown:     tc.clowntown,
				ImageFilePath: imageFileName,
				DeviceID:      mockDeviceID,
			}

			got := validateImageMetaPartitions(stepParams)

			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
