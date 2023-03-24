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

package image

import (
	"bytes"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestValidateImageFileSize(t *testing.T) {
	getFileSizeOrig := fileutils.GetFileSize
	getFlashDeviceOrig := flashutils.GetFlashDevice
	defer func() {
		fileutils.GetFileSize = getFileSizeOrig
		flashutils.GetFlashDevice = getFlashDeviceOrig
	}()

	const mockImageFilePath = "/run/flashy/image"
	const mockDeviceID = "/dev/mtd5"

	cases := []struct {
		name           string
		imSize         int64
		imSizeErr      error
		flashDevice    devices.FlashDevice
		flashDeviceErr error
		want           error
	}{
		{
			name:   "Succeeded",
			imSize: 32,
			flashDevice: &devices.MemoryTechnologyDevice{
				FileSize: 32,
			},
		},
		{
			name:   "Image file too large",
			imSize: 34,
			flashDevice: &devices.MemoryTechnologyDevice{
				FileSize: 32,
			},
			want: errors.Errorf("Image size (34B) larger than flash device size (32B)"),
		},
		{
			name:      "Failed to get imSize",
			imSizeErr: errors.Errorf("failed"),
			want:      errors.Errorf("Unable to get size of image file '/run/flashy/image': failed"),
		},
		{
			name:           "Failed to get flash device",
			flashDeviceErr: errors.Errorf("failed"),
			want:           errors.Errorf("Unable to get flash device from deviceID '/dev/mtd5': failed"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.GetFileSize = func(filename string) (int64, error) {
				if filename != mockImageFilePath {
					t.Errorf("filename: want '%v' got '%v'", mockImageFilePath, filename)
				}
				return tc.imSize, tc.imSizeErr
			}
			flashutils.GetFlashDevice = func(deviceID string) (devices.FlashDevice, error) {
				if deviceID != mockDeviceID {
					t.Errorf("deviceID: want '%v' got '%v'", mockDeviceID, deviceID)
				}
				return tc.flashDevice, tc.flashDeviceErr
			}
			got := validateImageFileSize(mockImageFilePath, mockDeviceID)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestValidateImageFile(t *testing.T) {
	validateImageFileSizeOrig := validateImageFileSize
	mmapFileOrig := fileutils.MmapFile
	validateOrig := validate.Validate
	getFileSizeOrig := fileutils.GetFileSize
	defer func() {
		validateImageFileSize = validateImageFileSizeOrig
		fileutils.MmapFile = mmapFileOrig
		validate.Validate = validateOrig
		fileutils.GetFileSize = getFileSizeOrig
	}()

	const imageFileName = "/run/upgrade/image"
	imageData := []byte("abcd")

	cases := []struct {
		name          string
		maybeDeviceID string
		imSizeErr     error
		mmapErr       error
		validateErr   error
		want          error
	}{
		{
			name:        "succeeded",
			mmapErr:     nil,
			validateErr: nil,
			want:        nil,
		},
		{
			name:        "mmap error",
			mmapErr:     errors.Errorf("mmap error"),
			validateErr: nil,
			want: errors.Errorf("Unable to read image file '%v': %v",
				"/run/upgrade/image", "mmap error"),
		},
		{
			name:        "validation error",
			mmapErr:     nil,
			validateErr: errors.Errorf("validation failed"),
			want:        errors.Errorf("validation failed"),
		},
		{
			name:      "file size not called because maybeDeviceID is empty",
			imSizeErr: errors.Errorf("Should not be called"),
		},
		{
			name:          "file size error",
			maybeDeviceID: "mtd:flash0",
			imSizeErr:     errors.Errorf("Failed"),
			want:          errors.Errorf("Image file size check failed: Failed"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			validateImageFileSize = func(imageFilePath string, deviceID string) error {
				if imageFilePath != imageFileName {
					t.Errorf("imageFilePath: want '%v' got '%v'", imageFilePath, imageFileName)
				}
				if deviceID != tc.maybeDeviceID {
					t.Errorf("deviceID: want '%v' got '%v'", tc.maybeDeviceID, deviceID)
				}
				return tc.imSizeErr
			}
			fileutils.MmapFile = func(filename string, prot, flags int) ([]byte, error) {
				if filename != imageFileName {
					t.Errorf("filename: want '%v' got '%v'", imageFileName, filename)
				}
				return imageData, tc.mmapErr
			}
			validate.Validate = func(data []byte) error {
				if !bytes.Equal(data, imageData) {
					t.Errorf("data: want '%v' got '%v'", imageData, data)
				}
				return tc.validateErr
			}

			got := ValidateImageFile(imageFileName, tc.maybeDeviceID)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
