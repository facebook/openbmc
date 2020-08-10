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

package flashutils

import (
	"bytes"
	"log"
	"os"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

type mockFlashDevice struct {
	ReadErr error
	devices.FlashDevice
}

func (m mockFlashDevice) MmapRO() ([]byte, error) {
	return []byte{'t', 'e', 's', 't'}, m.ReadErr
}

func (m mockFlashDevice) Munmap(b []byte) error {
	return nil
}

func (m mockFlashDevice) GetFileSize() uint64 {
	return uint64(4)
}

func TestPatchImageWithLocalBootloader(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore WriteFileWithoutTruncate
	writeFileOrig := fileutils.WriteFileWithoutTruncate
	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.WriteFileWithoutTruncate = writeFileOrig
	}()
	const imageFilePath = "imageFilePath"
	const flashDevicePath = "flashDevicePath"

	cases := []struct {
		name               string
		offsetBytes        int
		flashDeviceReadErr error
		writeImgFileErr    error
		logContainsSeq     []string
		want               error
	}{
		{
			name:               "patch success",
			offsetBytes:        2,
			flashDeviceReadErr: nil,
			writeImgFileErr:    nil,
			logContainsSeq: []string{
				"===== WARNING: PATCHING IMAGE FILE =====",
				"This vboot system has 2B RO offset in mtd, patching image file with offset.",
				"Successfully patched image file",
			},
			want: nil,
		},
		{
			name:               "patch failed (can't read from flash device)",
			offsetBytes:        2,
			flashDeviceReadErr: errors.Errorf("can't read from flash device"),
			writeImgFileErr:    nil,
			logContainsSeq:     []string{},
			want: errors.Errorf("Unable to patch image: Can't read from flash device: " +
				"can't read from flash device"),
		},
		{
			name:               "offset bytes larger than image",
			offsetBytes:        12,
			flashDeviceReadErr: nil,
			writeImgFileErr:    nil,
			logContainsSeq:     []string{},
			want: errors.Errorf("Unable to patch image: offset bytes required (%vB) larger than"+
				"image file size (%vB)", 12, 4),
		},
		{
			name:               "can't write to image file",
			offsetBytes:        2,
			flashDeviceReadErr: nil,
			writeImgFileErr:    errors.Errorf("write img error"),
			logContainsSeq:     []string{},
			want:               errors.Errorf("Unable to patch image file 'imageFilePath': write img error"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			fileutils.WriteFileWithoutTruncate = func(filename string, buf []byte) error {
				if filename != imageFilePath {
					t.Errorf("filename: want '%v' got '%v'", imageFilePath, filename)
				}
				return tc.writeImgFileErr
			}
			mockFD := mockFlashDevice{
				tc.flashDeviceReadErr,
				nil,
			}
			got := patchImageWithLocalBootloader(imageFilePath, mockFD, tc.offsetBytes)

			tests.CompareTestErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestIsVbootImagePatchingRequired(t *testing.T) {
	// mock and defer restore GetVbootEnforcement
	getVbootEnforcementOrig := utils.GetVbootEnforcement
	defer func() {
		utils.GetVbootEnforcement = getVbootEnforcementOrig
	}()
	cases := []struct {
		name                 string
		vbootEnforcement     utils.VbootEnforcementType
		vbootEnforcementErr  error
		flashDeviceSpecifier string
		want                 bool
		wantErr              error
	}{
		{
			name:                 "No vboot enforcement",
			vbootEnforcement:     utils.VBOOT_NONE,
			vbootEnforcementErr:  nil,
			flashDeviceSpecifier: "flash1",
			want:                 false,
			wantErr:              nil,
		},
		{
			name:                 "software vboot enforcement",
			vbootEnforcement:     utils.VBOOT_SOFTWARE_ENFORCE,
			vbootEnforcementErr:  nil,
			flashDeviceSpecifier: "flash1",
			want:                 false,
			wantErr:              nil,
		},
		{
			name:                 "hardware vboot enforcement, flash1",
			vbootEnforcement:     utils.VBOOT_HARDWARE_ENFORCE,
			vbootEnforcementErr:  nil,
			flashDeviceSpecifier: "flash1",
			want:                 true,
			wantErr:              nil,
		},
		{
			name:                 "hardware vboot enforcement, flash0 (not required)",
			vbootEnforcement:     utils.VBOOT_HARDWARE_ENFORCE,
			vbootEnforcementErr:  nil,
			flashDeviceSpecifier: "flash0",
			want:                 false,
			wantErr:              nil,
		},
		{
			name:                 "error getting vboot enforcement",
			vbootEnforcement:     utils.VBOOT_NONE,
			vbootEnforcementErr:  errors.Errorf("vboot enforcement err"),
			flashDeviceSpecifier: "flash0",
			want:                 false,
			wantErr:              errors.Errorf("Unable to get vboot enforcement: vboot enforcement err"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetVbootEnforcement = func() (utils.VbootEnforcementType, error) {
				return tc.vbootEnforcement, tc.vbootEnforcementErr
			}
			got, err := isVbootImagePatchingRequired(tc.flashDeviceSpecifier)
			if tc.want != got {
				t.Errorf("want %v got %v", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestVbootPatchImageBootloaderIfNeeded(t *testing.T) {
	// mock and defer restore IsVbootSystem, isVbootImagePatchingRequired
	// and patchImageWithLocalBootloader
	isVbootSystemOrig := utils.IsVbootSystem
	isVbootImagePatchingRequiredOrig := isVbootImagePatchingRequired
	patchImageWithLocalBootloaderOrig := patchImageWithLocalBootloader
	defer func() {
		utils.IsVbootSystem = isVbootSystemOrig
		isVbootImagePatchingRequired = isVbootImagePatchingRequiredOrig
		patchImageWithLocalBootloader = patchImageWithLocalBootloaderOrig
	}()

	exampleFlashDevice := &devices.MemoryTechnologyDevice{
		"flash1",
		"/dev/mtd5",
		uint64(42),
	}

	cases := []struct {
		name             string
		isVboot          bool
		patchRequired    bool
		patchRequiredErr error
		patchErr         error
		want             error
	}{
		{
			name:             "vboot patch required & succeeded",
			isVboot:          true,
			patchRequired:    true,
			patchRequiredErr: nil,
			patchErr:         nil,
			want:             nil,
		},
		{
			name:             "vboot patch not required",
			isVboot:          true,
			patchRequired:    false,
			patchRequiredErr: nil,
			patchErr:         nil,
			want:             nil,
		},
		{
			name:             "not a vboot system",
			isVboot:          false,
			patchRequired:    false,
			patchRequiredErr: nil,
			patchErr:         nil,
			want:             errors.Errorf("Not a vboot system, cannot run vboot remediation"),
		},
		{
			name:             "patch required check failed",
			isVboot:          true,
			patchRequired:    false,
			patchRequiredErr: errors.Errorf("check fail"),
			patchErr:         nil,
			want:             errors.Errorf("Unable to determine whether image patching is required: check fail"),
		},
		{
			name:             "patch required and failed",
			isVboot:          true,
			patchRequired:    true,
			patchRequiredErr: nil,
			patchErr:         errors.Errorf("patch fail"),
			want:             errors.Errorf("Failed to patch image with local bootloader: patch fail"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.IsVbootSystem = func() bool {
				return tc.isVboot
			}
			isVbootImagePatchingRequired = func(flashDeviceSpecifier string) (bool, error) {
				if flashDeviceSpecifier != exampleFlashDevice.Specifier {
					t.Errorf("flash device specifier: want '%v' got '%v'",
						exampleFlashDevice.Specifier, flashDeviceSpecifier)
				}
				return tc.patchRequired, tc.patchRequiredErr
			}
			patchImageWithLocalBootloader = func(imageFilePath string, flashDevice devices.FlashDevice, offsetBytes int) error {
				if imageFilePath != "x" {
					t.Errorf("image file path: want 'x' got '%v'", imageFilePath)
				}
				if offsetBytes != vbootOffset {
					t.Errorf("offsetBytes: want '%v' got '%v'", vbootOffset, offsetBytes)
				}
				return tc.patchErr
			}
			got := VbootPatchImageBootloaderIfNeeded("x", exampleFlashDevice)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
