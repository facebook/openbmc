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
	isLfOrig := utils.IsLFOpenBMC

	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.FileExists = fileExistsOrig
		utils.RunCommand = runCommandOrig
		utils.IsLFOpenBMC = isLfOrig
	}()

	cases := []struct {
		name              string
		vbootUtilExists   bool
		lfOpenBMC	        bool
		failGrep          bool
		failLs            bool
		failPrint         bool
		failSet           bool
		lsOutput          string
		printOutput       string
		grepOutput        string
		want              step.StepExitError
	}{
		{
			name:              "vboot case",
			vbootUtilExists:   true,
			lfOpenBMC:         false,
			failGrep:          false,
			failLs:            false,
			failPrint:         false,
			failSet:           false,
			lsOutput:          "/dev/mtd1",
			printOutput:       "derp",
			grepOutput:        "foo",
			want:              nil,
		},
		{
			name:              "lf-openbmc case",
			vbootUtilExists:   false,
			lfOpenBMC:         true,
			failGrep:          false,
			failLs:            false,
			failPrint:         false,
			failSet:           false,
			lsOutput:          "/dev/mtd1",
			printOutput:       "derp",
			grepOutput:        "foo",
			want:              nil,
		},
		{
			name:              "non-vboot case",
			vbootUtilExists:   false,
			lfOpenBMC:         false,
			failGrep:          false,
			failLs:            false,
			failPrint:         false,
			failSet:           false,
			lsOutput:          "/dev/mtd1",
			printOutput:       "bootargs=console=ttyS2,9600n8 root=/dev/ram rw",
			grepOutput:        "mtd4: 02000000 00010000 \"flash0\"",
			want:              nil,
		},
		{
			name:              "remediation case",
			vbootUtilExists:   false,
			lfOpenBMC:         false,
			failGrep:          true,
			failLs:            false,
			failPrint:         false,
			failSet:           false,
			lsOutput:          "/dev/mtd1",
			printOutput:       "bootargs=console=ttyS2,9600n8 root=/dev/ram rw",
			grepOutput:        "",
			want:              step.ExitMustReboot{Err: errors.Errorf("Forcing reboot for new bootargs to take effect")},
		},
		{
			name:              "missing flash",
			vbootUtilExists:   false,
			lfOpenBMC:         false,
			failGrep:          false,
			failLs:            true,
			failPrint:         false,
			failSet:           false,
			lsOutput:          "Cannot access MTD device /dev/mtd1: No such file or directory",
			printOutput:       "",
			grepOutput:        "",
			want:              step.ExitMissingMtd{Err: errors.Errorf("Broken flash chip? Cannot see MTD chips on device. Error code: err1, stderr: Cannot access MTD device /dev/mtd1: No such file or directory")},
		},
		{
			name:              "fw_printenv broken",
			vbootUtilExists:   false,
			lfOpenBMC:         false,
			failGrep:          false,
			failLs:            false,
			failPrint:         true,
			failSet:           false,
			lsOutput:          "/dev/mtd1",
			printOutput:       "Cannot access MTD device /dev/mtd1: No such file or directory",
			grepOutput:        "",
			want:              step.ExitBadFlashChip{Err: errors.Errorf("Broken flash chip? U-Boot environment is inaccessible. Error code: err1, stderr: Cannot access MTD device /dev/mtd1: No such file or directory")},
		},
		{
			name:              "fw_setenv broken",
			vbootUtilExists:   false,
			lfOpenBMC:         false,
			failGrep:          false,
			failLs:            false,
			failPrint:         false,
			failSet:           true,
			lsOutput:          "/dev/mtd1",
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
				} else if (cmdArr[0] == "ls") {
					if (tc.failLs) {
						return 1, errors.Errorf("err1"), "", tc.lsOutput
					} else {
						return 0, nil, "", tc.lsOutput
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
			utils.IsLFOpenBMC = func() (bool) {
				return tc.lfOpenBMC
			}
			got := ensureFlashAvailable(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
