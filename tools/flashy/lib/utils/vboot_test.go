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

package utils

import (
	"fmt"
	"strings"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

type partialRunCommandReturn struct {
	err    error
	stdout string
}

func TestGetVbootUtilContents(t *testing.T) {
	// mock and defer restore RemoveFile, RunCommand, IsVbootSystem and
	// IsELFFile
	removeFileOrig := fileutils.RemoveFile
	runCommandOrig := RunCommand
	isVbootSystemOrig := IsVbootSystem
	isELFFileOrig := fileutils.IsELFFile
	defer func() {
		fileutils.RemoveFile = removeFileOrig
		RunCommand = runCommandOrig
		IsVbootSystem = isVbootSystemOrig
		fileutils.IsELFFile = isELFFileOrig
	}()

	// removeFile is mocked to return nil
	fileutils.RemoveFile = func(_ string) error {
		return nil
	}

	cases := []struct {
		name          string
		isVbootSystem bool
		isELF         bool
		runCmdRet     partialRunCommandReturn
		wantCmd       string
		want          string
		wantErr       error
	}{
		{
			name:          "not a vboot system",
			isVbootSystem: false,
			isELF:         true,
			runCmdRet:     partialRunCommandReturn{},
			wantCmd:       "",
			want:          "",
			wantErr:       errors.Errorf("Not a vboot system"),
		},
		{
			name:          "is ELF file",
			isVbootSystem: true,
			isELF:         true,
			runCmdRet: partialRunCommandReturn{
				nil,
				"foobar",
			},
			wantCmd: vbootUtilPath,
			want:    "foobar",
			wantErr: nil,
		},
		{
			name:          "not ELF",
			isVbootSystem: true,
			isELF:         false,
			runCmdRet: partialRunCommandReturn{
				nil,
				"foobar",
			},
			wantCmd: fmt.Sprintf("bash %v", vbootUtilPath),
			want:    "foobar",
			wantErr: nil,
		},
		{
			name:          "failed",
			isVbootSystem: true,
			isELF:         true,
			runCmdRet: partialRunCommandReturn{
				errors.Errorf("cmd err"),
				"",
			},
			wantCmd: vbootUtilPath,
			want:    "",
			wantErr: errors.Errorf("Unable to get vboot-util info: cmd err"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			gotCmd := ""
			IsVbootSystem = func() bool {
				return tc.isVbootSystem
			}
			RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
				gotCmd = strings.Join(cmdArr, " ")
				retErr := tc.runCmdRet.err
				stdout := tc.runCmdRet.stdout
				return 0, retErr, stdout, ""
			}
			fileutils.IsELFFile = func(filename string) bool {
				if filename != vbootUtilPath {
					t.Errorf("want vboot util path '%v' got '%v'", vbootUtilPath, filename)
				}
				return tc.isELF
			}
			got, err := getVbootUtilContents()

			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
			if tc.wantCmd != gotCmd {
				t.Errorf("want cmd '%v' got '%v'", tc.wantCmd, gotCmd)
			}
		})
	}
}

func TestGetVbootEnforcement(t *testing.T) {
	// mock and defer restore IsVbootSystem, ReadFile and getVbootUtilContents
	isVbootSystemOrig := IsVbootSystem
	readFileOrig := fileutils.ReadFile
	getVbootUtilContentsOrig := getVbootUtilContents
	defer func() {
		IsVbootSystem = isVbootSystemOrig
		fileutils.ReadFile = readFileOrig
		getVbootUtilContents = getVbootUtilContentsOrig
	}()

	cases := []struct {
		name              string
		isVbootSystem     bool
		procMtdContents   string
		procMtdReadErr    error
		vbootUtilContents string
		vbootUtilGetErr   error
		want              VbootEnforcementType
		wantErr           error
	}{
		{
			name:              "Not a vboot system",
			isVbootSystem:     false,
			procMtdContents:   "",
			procMtdReadErr:    nil,
			vbootUtilContents: "",
			vbootUtilGetErr:   nil,
			want:              VBOOT_NONE,
			wantErr:           nil,
		},
		{
			name:              "example wedge100 /proc/mtd, no romx, mock vboot system",
			isVbootSystem:     true,
			procMtdContents:   tests.ExampleWedge100ProcMtdFile,
			procMtdReadErr:    nil,
			vbootUtilContents: "",
			vbootUtilGetErr:   nil,
			want:              VBOOT_NONE,
			wantErr:           nil,
		},
		{
			name:              "tiogapass1 example",
			isVbootSystem:     true,
			procMtdContents:   tests.ExampleTiogapass1ProcMtdFile,
			procMtdReadErr:    nil,
			vbootUtilContents: tests.ExampleTiogapass1VbootUtilFile,
			vbootUtilGetErr:   nil,
			want:              VBOOT_HARDWARE_ENFORCE,
			wantErr:           nil,
		},
		{
			name:              "/proc/mtd read err",
			isVbootSystem:     true,
			procMtdContents:   "",
			procMtdReadErr:    errors.Errorf("proc mtd read err"),
			vbootUtilContents: "",
			vbootUtilGetErr:   nil,
			want:              VBOOT_NONE,
			wantErr:           errors.Errorf("Unable to read '/proc/mtd': proc mtd read err"),
		},
		{
			name:              "getVbootUtilContents err",
			isVbootSystem:     true,
			procMtdContents:   "romx",
			procMtdReadErr:    nil,
			vbootUtilContents: "",
			vbootUtilGetErr:   errors.Errorf("getVbootUtilContents err"),
			want:              VBOOT_NONE,
			wantErr:           errors.Errorf("Unable to read vboot-util contents: getVbootUtilContents err"),
		},
		{
			name:            "software enforce example",
			isVbootSystem:   true,
			procMtdContents: "romx",
			procMtdReadErr:  nil,
			vbootUtilContents: `Flags hardware_enforce:  0x00
Flags software_enforce:  0x01`,
			vbootUtilGetErr: nil,
			want:            VBOOT_SOFTWARE_ENFORCE,
			wantErr:         nil,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			IsVbootSystem = func() bool {
				return tc.isVbootSystem
			}
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != ProcMtdFilePath {
					t.Errorf("filename: want '%v' got '%v'",
						ProcMtdFilePath, filename)
				}
				return []byte(tc.procMtdContents), tc.procMtdReadErr
			}
			getVbootUtilContents = func() (string, error) {
				return tc.vbootUtilContents, tc.vbootUtilGetErr
			}
			got, err := GetVbootEnforcement()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}
