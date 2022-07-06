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

func TestEnsureFlashAvailable(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	fileExistsOrig := fileutils.FileExists
	runCommandOrig := utils.RunCommand

	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.FileExists = fileExistsOrig
		utils.RunCommand = runCommandOrig
	}()

	cases := []struct {
		name              string
		vbootUtilExists   bool
		failGrep          bool
		failPrint         bool
		failSet           bool
		printOutput       string
		grepOutput        string
		want              step.StepExitError
	}{
		{
			name:              "vboot case",
			vbootUtilExists:   true,
			failGrep:          false,
			failPrint:         false,
			failSet:           false,
			printOutput:       "derp",
			grepOutput:        "foo",
			want:              nil,
		},
		{
			name:              "non-vboot case",
			vbootUtilExists:   false,
			failGrep:          false,
			failPrint:         false,
			failSet:           false,
			printOutput:       "bootargs=console=ttyS2,9600n8 root=/dev/ram rw",
			grepOutput:        "mtd4: 02000000 00010000 \"flash0\"",
			want:              nil,
		},
		{
			name:              "remediation case",
			vbootUtilExists:   false,
			failGrep:          true,
			failPrint:         false,
			failSet:           false,
			printOutput:       "bootargs=console=ttyS2,9600n8 root=/dev/ram rw",
			grepOutput:        "",
			want:              step.ExitMustReboot{Err: errors.Errorf("Forcing reboot for new bootargs to take effect")},
		},
		{
			name:              "fw_printenv broken because reasons",
			vbootUtilExists:   false,
			failGrep:          false,
			failPrint:         true,
			failSet:           false,
			printOutput:       "",
			grepOutput:        "",
			want:              nil,
		},
		{
			name:              "fw_printenv broken because missing flash",
			vbootUtilExists:   false,
			failGrep:          false,
			failPrint:         true,
			failSet:           false,
			printOutput:       "Cannot access MTD device /dev/mtd1: No such file or directory",
			grepOutput:        "",
			want:              step.ExitBadFlashChip{Err: errors.Errorf("U-Boot environment is inaccessible: broken flash chip? Error code: err1, stderr: Cannot access MTD device /dev/mtd1: No such file or directory")},
		},
		{
			name:              "fw_setenv broken",
			vbootUtilExists:   false,
			failGrep:          false,
			failPrint:         false,
			failSet:           true,
			printOutput:       "bootargs=console=ttyS2,9600n8 root=/dev/ram rw",
			grepOutput:        "",
			want:              nil,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.FileExists = func(filename string) bool {
				return tc.vbootUtilExists
			}
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				if (cmdArr[0] == "grep") {
					if (tc.failGrep) {
						return 1, nil, "", "bmc derped"
					} else {
						return 0, nil, tc.grepOutput, ""
					}
				} else if (cmdArr[0] == "fw_printenv") {
					if (tc.failPrint) {
						return 1, errors.Errorf("err1"), "", tc.printOutput
					} else {
						return 0, nil, "", tc.printOutput
					}
				} else if (cmdArr[0] == "fw_setenv") {
					if (tc.failSet) {
						return 0, errors.Errorf("err2"), "", "err2"
					} else {
						return 0, nil, "", ""
					}
				}
				return 0, errors.Errorf("err3"), "", "err3"
			}
			got := ensureFlashAvailable(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
