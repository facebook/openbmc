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
	// mock and defer restore GetFlashDevice, RunVbootRemediation and runFlashCpCmd
	getFlashDeviceOrig := flashutils.GetFlashDevice
	vbootPatchOrig := flashutils.VbootPatchImageBootloaderIfNeeded
	runFlashCpCmdOrig := runFlashCpCmd
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
		flashutils.VbootPatchImageBootloaderIfNeeded = vbootPatchOrig
		runFlashCpCmd = runFlashCpCmdOrig
	}()

	exampleFlashDevice := devices.MemoryTechnologyDevice{
		"flash0",
		"/dev/mtd5",
		uint64(12345678),
	}

	exampleStepParams := step.StepParams{
		false,
		"/tmp/image",
		"mtd:flash0",
		false,
	}

	cases := []struct {
		name             string
		flashDevice      devices.FlashDevice
		flashDeviceErr   error
		vbootRemErr      error
		runFlashCpCmdErr step.StepExitError
		want             step.StepExitError
		logContainsSeq   []string
	}{
		{
			name:             "basic successful flash",
			flashDevice:      exampleFlashDevice,
			flashDeviceErr:   nil,
			vbootRemErr:      nil,
			runFlashCpCmdErr: nil,
			want:             nil,
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: {flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:             "failed to get flash target file",
			flashDevice:      nil,
			flashDeviceErr:   errors.Errorf("GetFlashDevice error"),
			vbootRemErr:      nil,
			runFlashCpCmdErr: nil,
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
			name:             "runFlashCpCmd failed",
			flashDevice:      exampleFlashDevice,
			flashDeviceErr:   nil,
			vbootRemErr:      nil,
			runFlashCpCmdErr: step.ExitSafeToReboot{errors.Errorf("RunCommand error")},
			want: step.ExitSafeToReboot{
				errors.Errorf("RunCommand error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: {flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:             "vboot rem failed",
			flashDevice:      exampleFlashDevice,
			flashDeviceErr:   nil,
			vbootRemErr:      errors.Errorf("vboot rem failed"),
			runFlashCpCmdErr: nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("vboot rem failed"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp vboot method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
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
				return tc.flashDevice, tc.flashDeviceErr
			}
			flashutils.VbootPatchImageBootloaderIfNeeded = func(imageFilePath string, flashDevice devices.FlashDevice) error {
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				return tc.vbootRemErr
			}
			runFlashCpCmd = func(imageFilePath, flashDevicePath string) step.StepExitError {
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				if flashDevicePath != "/dev/mtd5" {
					t.Errorf("flashDevicePath: want '%v' got '%v'", "/dev/mtd5", flashDevicePath)
				}
				return tc.runFlashCpCmdErr
			}
			got := FlashCpVboot(exampleStepParams)

			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
