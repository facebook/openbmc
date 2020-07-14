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
	"strings"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/flash/flashutils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestFlashCp(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore GetFlashDevice and RunCommand
	getFlashDeviceOrig := flashutils.GetFlashDevice
	runCommandOrig := utils.RunCommand
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
		utils.RunCommand = runCommandOrig
	}()

	cases := []struct {
		name           string
		imageFilePath  string
		deviceID       string
		flashDevice    *devices.FlashDevice
		flashDeviceErr error
		runCmdExitCode int
		runCmdErr      error
		want           utils.StepExitError
		wantCmd        string
		logContainsSeq []string
	}{
		{
			name:          "basic successful flash",
			imageFilePath: "/tmp/image",
			deviceID:      "mtd:flash0",
			flashDevice: &devices.FlashDevice{
				"mtd",
				"flash0",
				"/dev/mtd5",
				uint64(12345678),
				uint64(0),
			},
			flashDeviceErr: nil,
			runCmdExitCode: 0,
			runCmdErr:      nil,
			want:           nil,
			wantCmd:        "flashcp /tmp/image /dev/mtd5",
			logContainsSeq: []string{
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Using legacy flash method (flashcp)",
				"Gotten flash device: {mtd flash0 /dev/mtd5 12345678 0}",
				"Flashing succeeded, safe to reboot",
			},
		},
		{
			name:           "failed to get flash target file",
			imageFilePath:  "/tmp/image",
			deviceID:       "mtd:flash0",
			flashDevice:    nil,
			flashDeviceErr: errors.Errorf("GetFlashDevice error"),
			runCmdExitCode: 0,
			runCmdErr:      nil,
			want: utils.ExitSafeToReboot{
				errors.Errorf("GetFlashDevice error"),
			},
			wantCmd: "",
			logContainsSeq: []string{
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Using legacy flash method (flashcp)",
				"GetFlashDevice error",
			},
		},
		{
			name:          "command failed",
			imageFilePath: "/tmp/image",
			deviceID:      "mtd:flash0",
			flashDevice: &devices.FlashDevice{
				"mtd",
				"flash0",
				"/dev/mtd5",
				uint64(12345678),
				uint64(0),
			},
			flashDeviceErr: nil,
			runCmdExitCode: 1,
			runCmdErr:      errors.Errorf("RunCommand error"),
			want: utils.ExitSafeToReboot{
				errors.Errorf("Flashing failed with exit code 1, error: RunCommand error"),
			},
			wantCmd: "flashcp /tmp/image /dev/mtd5",
			logContainsSeq: []string{
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Using legacy flash method (flashcp)",
				"Gotten flash device: {mtd flash0 /dev/mtd5 12345678 0}",
				"Flashing failed with exit code 1, error: RunCommand error",
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			var gotCmd string
			flashutils.GetFlashDevice = func(deviceID string) (*devices.FlashDevice, error) {
				return tc.flashDevice, tc.flashDeviceErr
			}
			utils.RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
				gotCmd = strings.Join(cmdArr, " ")
				return tc.runCmdExitCode, tc.runCmdErr, "", ""
			}

			got := FlashCp(tc.imageFilePath, tc.deviceID)

			tests.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)

			if tc.wantCmd != gotCmd {
				t.Errorf("RunCommand: want '%v' got '%v'", tc.wantCmd, gotCmd)
			}
		})
	}
}
