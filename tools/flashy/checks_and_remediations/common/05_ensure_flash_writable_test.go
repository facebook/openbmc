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
	"bytes"
	"log"
	"os"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func TestEnsureFlashWritable(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	readFileOrig := fileutils.ReadFile
	fileExistsOrig := fileutils.FileExists
	runCommandOrig := utils.RunCommand

	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.ReadFile = readFileOrig
		fileutils.FileExists = fileExistsOrig
		utils.RunCommand = runCommandOrig
	}()

	cases := []struct {
		name              string
		vbootUtilExists   bool
		s356523						string
		failPrint         bool
		failSet           bool
		failCheck         bool
		failClear         bool
		want              step.StepExitError
	}{
		{
			name:              "non-vboot case",
			vbootUtilExists:   false,
			s356523:					 "",
			failPrint:         false,
			failSet:           false,
			failCheck:         false,
			failClear:         false,
			want:              nil,
		},
		{
			name:              "working case",
			vbootUtilExists:   true,
			s356523:					 "",
			failPrint:         false,
			failSet:           false,
			failCheck:         false,
			failClear:         false,
			want:              nil,
		},
		{
			name:              "s356523 case",
			vbootUtilExists:   true,
			s356523:					 `mtd0: 08000000 00010000 "spi0.1"
			mtd1: 00040000 00010000 "romx"`,
			failPrint:         false,
			failSet:           false,
			failCheck:         false,
			failClear:         false,
			want:              nil,
		},
		{
			name:              "fw_printenv broken",
			vbootUtilExists:   true,
			s356523:					 "",
			failPrint:         true,
			failSet:           false,
			failCheck:         false,
			failClear:         false,
			want:              nil,
		},
		{
			name:              "set _flashy_test fails",
			vbootUtilExists:   true,
			s356523:					 "",
			failPrint:         false,
			failSet:           true,
			failCheck:         false,
			failClear:         false,
			want:              nil,
		},
		{
			name:              "check _flashy_test fails",
			vbootUtilExists:   true,
			s356523:					 "",
			failPrint:         false,
			failSet:           false,
			failCheck:         true,
			failClear:         false,
			want:              step.ExitBadFlashChip{
                                Err: errors.Errorf("U-Boot environment is read only: flash chips swapped? Error code: <nil>, stderr: ")},
		},
		{
			name:              "clear _flashy_test fails",
			vbootUtilExists:   true,
			s356523:					 "",
			failPrint:         false,
			failSet:           false,
			failCheck:         false,
			failClear:         true,
			want:              nil,
		},
	}

	key := "_fake_key"
	value := "_fake_value"

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.FileExists = func(filename string) bool {
				return tc.vbootUtilExists
			}
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				return []byte(tc.s356523), nil
			}
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				if (cmdArr[0] == "fw_printenv") {
					if (cmdArr[1] == "bootargs") {
						if (tc.failPrint) {
							return 0, nil, "", ""
						} else {
							return 0, nil, "bootargs=foo", ""
						}
					} else if (cmdArr[1] == key) {
						if (tc.failCheck) {
							return 0, nil, "", ""
						} else {
							return 0, nil, key + "=" + value, ""
						}
					}
				} else if (cmdArr[0] == "fw_setenv") {
					if (len(cmdArr) <= 2) {
						if (tc.failClear) {
							return 0, errors.Errorf("err1"), "", "err1"
						} else {
							return 0, nil, "bootargs=foo", ""
						}
					} else{
						if (tc.failSet) {
							return 0, errors.Errorf("err2"), "", "err2"
						} else {
							key = cmdArr[1]
							value = cmdArr[2]
							return 0, nil, "", ""
						}
					}
				}
				return 0, errors.Errorf("err3"), "", "err3"
			}
			got := ensureFlashWritable(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
