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

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashcp"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

type mockFlashDevice struct {
	ValidationErr error
}

func (m *mockFlashDevice) GetType() string         { return "mocktype" }
func (m *mockFlashDevice) GetSpecifier() string    { return "mockspec" }
func (m *mockFlashDevice) GetFilePath() string     { return "/dev/mock" }
func (m *mockFlashDevice) GetFileSize() uint64     { return uint64(1234) }
func (m *mockFlashDevice) MmapRO() ([]byte, error) { return nil, nil }
func (m *mockFlashDevice) Munmap([]byte) error     { return nil }
func (m *mockFlashDevice) Validate() error         { return m.ValidationErr }

func TestFlashCp(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore GetFlashDevice, flashCpAndValidate,
	// CheckOtherFlasherRunning
	getFlashDeviceOrig := flashutils.GetFlashDevice
	flashCpAndValidateOrig := flashCpAndValidate
	checkOtherFlasherRunningOrig := utils.CheckOtherFlasherRunning
	defer func() {
		log.SetOutput(os.Stderr)
		flashutils.GetFlashDevice = getFlashDeviceOrig
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
		otherFlasherErr       error
		want                  step.StepExitError
		logContainsSeq        []string
	}{
		{
			name:                  "basic successful flash",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			otherFlasherErr:       nil,
			want:                  nil,
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: &{flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:                  "failed to get flash device",
			getFlashDeviceErr:     errors.Errorf("GetFlashDevice error"),
			flashCpAndValidateErr: nil,
			otherFlasherErr:       nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("GetFlashDevice error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"GetFlashDevice error",
			},
		},
		{
			name:                  "flashcp and validate failed",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: errors.Errorf("RunCommand error"),
			otherFlasherErr:       nil,
			want: step.ExitUnsafeToReboot{
				Err: errors.Errorf("RunCommand error"),
			},
			logContainsSeq: []string{
				"Flashing using flashcp method",
				"Attempting to flash 'mtd:flash0' with image file '/tmp/image",
				"Flash device: &{flash0 /dev/mtd5 12345678}",
			},
		},
		{
			name:                  "other flasher running",
			getFlashDeviceErr:     nil,
			flashCpAndValidateErr: nil,
			otherFlasherErr:       errors.Errorf("Found other flasher!"),
			want: step.ExitUnsafeToReboot{
				Err: errors.Errorf("Found other flasher!"),
			},
			logContainsSeq: []string{
				"Flashing succeeded but found another flasher running",
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
			flashCpAndValidate = func(
				flashDevice devices.FlashDevice,
				imageFilePath string,
				offset uint32,
			) error {
				if !reflect.DeepEqual(flashDevice, exampleFlashDevice) {
					t.Errorf("flashDevice: want '%v' got '%v'", exampleFlashDevice, flashDevice)
				}
				if imageFilePath != "/tmp/image" {
					t.Errorf("imageFilePath: want '%v' got '%v'", "/tmp/image", imageFilePath)
				}
				if offset != 0 {
					t.Errorf("offset: want '%v' got '%v'", 0, offset)
				}
				return tc.flashCpAndValidateErr
			}
			utils.CheckOtherFlasherRunning = func(flashyStepBaseNames []string) error {
				return tc.otherFlasherErr
			}
			got := FlashCp(exampleStepParams)

			step.CompareTestExitErrors(tc.want, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestFlashCpAndValidate(t *testing.T) {
	// mock and defer restore flashcp.FlashCp
	flashCpOrig := flashcp.FlashCp
	removeFileOrig := fileutils.RemoveFile
	defer func() {
		flashcp.FlashCp = flashCpOrig
		fileutils.RemoveFile = removeFileOrig
	}()

	cases := []struct {
		name          string
		flashCpErr    error
		removeErr     error
		want          error
	}{
		{
			name:          "succeeded",
			flashCpErr:    nil,
			removeErr:     nil,
			want:          nil,
		},
		{
			name:          "flashCpErr error",
			flashCpErr:    errors.Errorf("flashing failed"),
			removeErr:     nil,
			want:          errors.Errorf("flashing failed"),
		},
		{
			name:          "succeeded, with remove failure",
			flashCpErr:    nil,
			removeErr:     errors.Errorf("aw nuts"),
			want:          nil,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleImageFilePath := "/img/mock"
			fileutils.RemoveFile = func(filename string) (error) {
				return tc.removeErr
			}
			flashcp.FlashCp = func(imageFilePath, flashDevicePath string, offset uint32) error {
				if offset != 42 {
					t.Errorf("offset: want '%v' got '%v'", 42, offset)
				}
				if exampleImageFilePath != imageFilePath {
					t.Errorf("imageFilePath: want '%v' got '%v'",
						exampleImageFilePath, imageFilePath)
				}
				if flashDevicePath != "/dev/mock" {
					t.Errorf("flashDevicePath: want '%v' got '%v'", "/dev/mock",
						flashDevicePath)
				}
				return tc.flashCpErr
			}
			flashDevice := &mockFlashDevice{
				ValidationErr: nil,
			}
			got := flashCpAndValidate(flashDevice, exampleImageFilePath, 42)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
