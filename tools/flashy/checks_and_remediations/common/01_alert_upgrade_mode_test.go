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
	"os"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestAlertUpgradeMode(t *testing.T) {
	// make sure errors are ignored
	wallAlertOrig := wallAlert
	updateMOTDOrig := updateMOTD
	defer func() {
		wallAlert = wallAlertOrig
		updateMOTD = updateMOTDOrig
	}()
	wallAlert = func() error {
		return errors.Errorf("Should be ignored")
	}
	updateMOTD = func() error {
		return errors.Errorf("Should be ignored")
	}

	got := alertUpgradeMode(step.StepParams{})
	step.CompareTestExitErrors(nil, got, t)
}

func TestWallAlert(t *testing.T) {
	// mock and defer restore RunCommand
	runCommandOrig := utils.RunCommand
	defer func() {
		utils.RunCommand = runCommandOrig
	}()
	cases := []struct {
		name      string
		runCmdErr error
		want      error
	}{
		{
			name:      "succeeded",
			runCmdErr: nil,
			want:      nil,
		},
		{
			name:      "run command failed",
			runCmdErr: errors.Errorf("command failed"),
			want:      errors.Errorf("Warning: `wall` alert failed: command failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				return 0, tc.runCmdErr, "", ""
			}
			got := wallAlert()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestUpdateMOTD(t *testing.T) {
	// mock and defer restore WriteFileWithTimeout
	writeFileOrig := fileutils.WriteFileWithTimeout
	fileExistsOrig := fileutils.FileExists
	renameFileOrig := fileutils.RenameFile
	defer func() {
		fileutils.WriteFileWithTimeout = writeFileOrig
		fileutils.FileExists = fileExistsOrig
		fileutils.RenameFile = renameFileOrig
	}()
	cases := []struct {
		name         string
		writeFileErr error
		renameErr    error
		backupExists bool
		want         error
	}{
		{
			name:         "succeeded",
			writeFileErr: nil,
			renameErr:    nil,
			backupExists: false,
			want:         nil,
		},
		{
			name:         "succeeded",
			writeFileErr: nil,
			renameErr:    errors.Errorf("oopsie daisy"),
			backupExists: false,
			want:         nil,
		},
		{
			name:         "succeeded",
			writeFileErr: nil,
			renameErr:    nil,
			backupExists: true,
			want:         nil,
		},
		{
			name:         "WriteFile failed",
			writeFileErr: errors.Errorf("WriteFile failed"),
			renameErr:    nil,
			backupExists: false,
			want:         errors.Errorf("Warning: MOTD update failed: WriteFile failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.FileExists = func(filename string) bool {
				return tc.backupExists
			}
			fileutils.RenameFile = func(from string, to string) error {
				return tc.renameErr
			}
			fileutils.WriteFileWithTimeout = func(filename string, data []byte, perm os.FileMode, timeout time.Duration) error {
				if "/etc/motd" != filename {
					t.Errorf("filename: want '%v' got '%v'", "/etc/motd", filename)
				}
				if string(data) != motdContents {
					t.Errorf("data: want '%v' got '%v'", motdContents, string(data))
				}
				return tc.writeFileErr
			}

			got := updateMOTD()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
