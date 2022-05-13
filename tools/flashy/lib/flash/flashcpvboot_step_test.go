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

package flash

import (
	"bytes"
	"log"
	"os"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestVbootRomxExists(t *testing.T) {
	// mock and defer restore GetVbootEnforcement
	getVbootEnforcementOrig := utils.GetVbootEnforcement
	defer func() {
		utils.GetVbootEnforcement = getVbootEnforcementOrig
	}()

	cases := []struct {
		name                 string
		flashDeviceSpecifier string
		vbootEnforcement     utils.VbootEnforcementType
		want                 bool
	}{
		{
			name:                 "exists",
			flashDeviceSpecifier: "flash1",
			vbootEnforcement:     utils.VBOOT_HARDWARE_ENFORCE,
			want:                 true,
		},
		{
			name:                 "not hardware enforce",
			flashDeviceSpecifier: "flash1",
			vbootEnforcement:     utils.VBOOT_SOFTWARE_ENFORCE,
			want:                 true, // Ignoring vboot enforcement, see vbootRomxExists for details
		},
		{
			name:                 "not flash1",
			flashDeviceSpecifier: "flash1rw",
			vbootEnforcement:     utils.VBOOT_HARDWARE_ENFORCE,
			want:                 false,
		},
		{
			name:                 "not enforce",
			flashDeviceSpecifier: "flash1",
			vbootEnforcement:     utils.VBOOT_NONE,
			want:                 true, // Ignoring vboot enforcement, see vbootRomxExists for details
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetVbootEnforcement = func() utils.VbootEnforcementType {
				return tc.vbootEnforcement
			}
			got := vbootRomxExists(tc.flashDeviceSpecifier)
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestFlashCpVboot(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore GetFlashDevice, flashCpAndValidate,
	// vbootROExists and CheckOtherFlasherRunning
	getFlashDeviceOrig := flashutils.GetFlashDevice
	isVbootSkipNeededOrig := vbootRomxExists
	flashCpAndValidateOrig := flashCpAndValidate
	checkOtherFlasherRunningOrig := utils.CheckOtherFlasherRunning
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
		vbootRomxExists = isVbootSkipNeededOrig
		flashCpAndValidate = flashCpAndValidateOrig
		utils.CheckOtherFlasherRunning = checkOtherFlasherRunningOrig
	}()

	exampleStepParams := step.StepParams{
		ImageFilePath: "/tmp/image",
		DeviceID:      "mtd:flash0",
	}

	exampleFlashDevice := &devices.MemoryTechnologyDevice{
		Specifier: "flash0",
		FilePath:  "/dev/mtd5",
		FileSize:  uint64(12345678),
	}

	cases := []struct {
		name                  string
		getFlashDeviceErr     error
		flashCpAndValidateErr error
		vbootSkipNeeded       bool
		otherFlasherErr       error
		want                  step.StepExitError
		logContainsSeq        []string
	}{
		{
			name:                  "basic successful flash",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			vbootSkipNeeded:       false,
			otherFlasherErr:       nil,
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
			vbootSkipNeeded:       false,
			otherFlasherErr:       nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("GetFlashDevice error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"GetFlashDevice error",
			},
		},
		{
			name:                  "flashcp and validate failed",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: errors.Errorf("RunCommand error"),
			vbootSkipNeeded:       false,
			otherFlasherErr:       nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("RunCommand error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: &{flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:                  "other flasher running",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			vbootSkipNeeded:       false,
			otherFlasherErr:       errors.Errorf("Found other flasher!"),
			want: step.ExitUnsafeToReboot{
				Err: errors.Errorf("Found other flasher!"),
			},
			logContainsSeq: []string{
				"Flashing succeeded but found another flasher running",
			},
		},
		{
			name:                  "vboot rom exists",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			vbootSkipNeeded:       true,
			otherFlasherErr:       nil,
			want:                  nil,
			logContainsSeq: []string{
				"ROM part exists in flash device. Skipping 86016B ROM region.",
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
			vbootRomxExists = func(flashDeviceSpecifier string) bool {
				if flashDeviceSpecifier != "flash0" {
					t.Errorf("flashDeviceSpecifier: want '%v' got '%v'",
						exampleFlashDevice.GetSpecifier(), flashDeviceSpecifier)
				}
				return tc.vbootSkipNeeded
			}
			flashCpAndValidate = func(
				flashDevice devices.FlashDevice,
				imageFilePath string,
				roOffset uint32,
			) error {
				if tc.vbootSkipNeeded {
					if roOffset != vbootRomxSize {
						t.Errorf("roOffset: want '%v' got '%v'", vbootRomxSize, roOffset)
					}
				} else {
					if roOffset != 0 {
						t.Errorf("roOffset: want '%v' got '%v'", 0, roOffset)
					}
				}
				if !reflect.DeepEqual(flashDevice, exampleFlashDevice) {
					t.Errorf("flashDevice: want '%v' got '%v'", exampleFlashDevice, flashDevice)
				}
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				return tc.flashCpAndValidateErr
			}
			utils.CheckOtherFlasherRunning = func(flashyStepBaseNames []string) error {
				return tc.otherFlasherErr
			}
			got := FlashCpVboot(exampleStepParams)

			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
