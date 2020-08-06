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
	"fmt"
	"log"
	"os"
	"strings"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestFlashCp(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore GetFlashDevice, runFlashCpCmd
	getFlashDeviceOrig := flashutils.GetFlashDevice
	runFlashCpCmdOrig := runFlashCpCmd
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
		runFlashCpCmd = runFlashCpCmdOrig
	}()

	exampleFlashDevice := devices.MemoryTechnologyDevice{
		"flash0",
		"/dev/mtd5",
		uint64(12345678),
	}

	exampleStepParams := step.StepParams{
		ImageFilePath: "/tmp/image",
		DeviceID:      "mtd:flash0",
	}

	cases := []struct {
		name             string
		flashDevice      devices.FlashDevice
		flashDeviceErr   error
		runFlashCpCmdErr step.StepExitError
		want             step.StepExitError
		logContainsSeq   []string
	}{
		{
			name:             "basic successful flash",
			flashDevice:      exampleFlashDevice,
			flashDeviceErr:   nil,
			runFlashCpCmdErr: nil,
			want:             nil,
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: {flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:             "failed to get flash target file",
			flashDevice:      nil,
			flashDeviceErr:   errors.Errorf("GetFlashDevice error"),
			runFlashCpCmdErr: nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("GetFlashDevice error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"GetFlashDevice error",
			},
		},
		{
			name:             "runFlashCpCmd failed",
			flashDevice:      exampleFlashDevice,
			flashDeviceErr:   nil,
			runFlashCpCmdErr: step.ExitSafeToReboot{errors.Errorf("RunCommand error")},
			want: step.ExitSafeToReboot{
				errors.Errorf("RunCommand error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: {flash0 /dev/mtd5 12345678}",
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
			runFlashCpCmd = func(imageFilePath, flashDevicePath string) step.StepExitError {
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				if flashDevicePath != "/dev/mtd5" {
					t.Errorf("flashDevicePath: want '%v' got '%v'", "/dev/mtd5", flashDevicePath)
				}
				return tc.runFlashCpCmdErr
			}
			got := FlashCp(exampleStepParams)

			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestRunFlashCpCmd(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore RunCommand
	runCommandOrig := utils.RunCommand
	defer func() {
		log.SetOutput(os.Stderr)
		utils.RunCommand = runCommandOrig
	}()

	type cmdRetType struct {
		exitCode int
		err      error
		stdout   string
		stderr   string
	}

	cases := []struct {
		name           string
		cmdRet         cmdRetType
		logContainsSeq []string
		want           step.StepExitError
	}{
		{
			name: "basic succeeding",
			cmdRet: cmdRetType{
				0, nil, "", "",
			},
			logContainsSeq: []string{
				"Flashing succeeded, safe to reboot",
			},
			want: nil,
		},
		{
			name: "cmd failed",
			cmdRet: cmdRetType{
				1,
				errors.Errorf("Flashcp failed"),
				"failed (stdout)",
				"failed (stderr)",
			},
			logContainsSeq: []string{
				fmt.Sprintf(
					"Flashing failed with exit code %v, error: %v, stdout: '%v', stderr: '%v'",
					1, "Flashcp failed", "failed (stdout)", "failed (stderr)",
				),
			},
			want: step.ExitSafeToReboot{
				errors.Errorf(
					"Flashing failed with exit code %v, error: %v, stdout: '%v', stderr: '%v'",
					1, "Flashcp failed", "failed (stdout)", "failed (stderr)",
				),
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			utils.RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
				cmdStr := strings.Join(cmdArr, " ")
				if cmdStr != "flashcp a b" {
					t.Errorf("cmd: want 'flashcp a b' got '%v'", cmdStr)
				}
				r := tc.cmdRet
				return r.exitCode, r.err, r.stdout, r.stderr
			}

			got := runFlashCpCmd("a", "b")
			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
