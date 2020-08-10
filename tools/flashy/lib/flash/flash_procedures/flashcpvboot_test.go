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

package flash_procedures

import (
	"bytes"
	"log"
	"os"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestFlashCpVboot(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore GetFlashDevice, flashCpAndValidate and
	// VbootPatchImageBootloaderIfNeeded
	getFlashDeviceOrig := flashutils.GetFlashDevice
	vbootPatchOrig := flashutils.VbootPatchImageBootloaderIfNeeded
	flashCpAndValidateOrig := flashCpAndValidate
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
		flashutils.VbootPatchImageBootloaderIfNeeded = vbootPatchOrig
		flashCpAndValidate = flashCpAndValidateOrig
	}()

	exampleStepParams := step.StepParams{
		ImageFilePath: "/tmp/image",
		DeviceID:      "mtd:flash0",
	}

	exampleFlashDevice := &devices.MemoryTechnologyDevice{
		"flash0",
		"/dev/mtd5",
		uint64(12345678),
	}

	cases := []struct {
		name                  string
		getFlashDeviceErr     error
		flashCpAndValidateErr error
		vbootPatchErr         error
		want                  step.StepExitError
		logContainsSeq        []string
	}{
		{
			name:                  "basic successful flash",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			vbootPatchErr:         nil,
			want:                  nil,
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: &{flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:                  "failed to get flash device",
			getFlashDeviceErr:     errors.Errorf("GetFlashDevice error"),
			flashCpAndValidateErr: nil,
			vbootPatchErr:         nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("GetFlashDevice error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"GetFlashDevice error",
			},
		},
		{
			name:                  "vboot patch error",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			vbootPatchErr:         errors.Errorf("failed to patch"),
			want: step.ExitSafeToReboot{
				errors.Errorf("failed to patch"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"failed to patch",
			},
		},
		{
			name:                  "flashcp and validate failed",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: errors.Errorf("RunCommand error"),
			vbootPatchErr:         nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("RunCommand error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: &{flash0 /dev/mtd5 12345678}",
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			flashutils.GetFlashDevice = func(deviceID string) (devices.FlashDevice, error) {
				if deviceID != "mtd:flash0" {
					t.Errorf("device: want '%v' got '%v'", "mtd:flash0", deviceID)
				}
				return exampleFlashDevice, tc.getFlashDeviceErr
			}
			flashutils.VbootPatchImageBootloaderIfNeeded = func(imageFilePath string, flashDevice devices.FlashDevice) error {
				if !reflect.DeepEqual(flashDevice, exampleFlashDevice) {
					t.Errorf("flashDevice: want '%v' got '%v'", exampleFlashDevice, flashDevice)
				}
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				return tc.vbootPatchErr
			}
			flashCpAndValidate = func(flashDevice devices.FlashDevice, imageFilePath string) error {
				if !reflect.DeepEqual(flashDevice, exampleFlashDevice) {
					t.Errorf("flashDevice: want '%v' got '%v'", exampleFlashDevice, flashDevice)
				}
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				return tc.flashCpAndValidateErr
			}
			got := FlashCpVboot(exampleStepParams)

			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
