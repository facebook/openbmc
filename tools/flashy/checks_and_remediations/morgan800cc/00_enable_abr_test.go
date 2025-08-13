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

func TestEnableABR(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	fileExistsOrig := fileutils.FileExists
	runCommandOrig := utils.RunCommand
	getOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile

	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.FileExists = fileExistsOrig
		utils.RunCommand = runCommandOrig
		utils.GetOpenBMCVersionFromIssueFile = getOpenBMCVersionFromIssueFileOrig
	}()

	cases := []struct {
		name          string
		otpCmdExists  bool
		versionString string
		versionError  error
		callFwSetEnv  bool
		ubootVars     string
		want          step.StepExitError
	}{
		{
			name:          "factory image",
			otpCmdExists:  false,
			versionString: "morgan800cc-65c81dc2d64",
			versionError:  nil,
			callFwSetEnv:  true,
			ubootVars:     "bootcmd=bootm 20100000",
			want:          step.ExitMustReboot{Err: errors.Errorf("Forcing reboot to enable ABR")},
		},
		{
			name:          "factory image, bootcmd already set",
			otpCmdExists:  false,
			versionString: "morgan800cc-65c81dc2d64",
			versionError:  nil,
			callFwSetEnv:  true,
			ubootVars:     "bootcmd=" + newBootCmd,
			want:          nil,
		},
		{
			name:          "factory image with otp command",
			otpCmdExists:  true,
			versionString: "morgan800cc-65c81dc2d64",
			versionError:  nil,
			callFwSetEnv:  false,
			ubootVars:     "bootcmd=bootm 20100000",
			want:          nil,
		},
		{
			name:          "meta image",
			otpCmdExists:  false,
			versionString: "morgan800cc-v2032.22.1",
			versionError:  nil,
			callFwSetEnv:  false,
			ubootVars:     "bootcmd=bootm 20100000",
			want:          nil,
		},
		{
			name:          "meta image with otp command",
			otpCmdExists:  true,
			versionString: "morgan800cc-v2032.22.1",
			versionError:  nil,
			callFwSetEnv:  false,
			ubootVars:     "bootcmd=bootm 20100000",
			want:          nil,
		},
		{
			name:          "failure to parse /etc/issue",
			otpCmdExists:  false,
			versionString: "unknown-v1",
			versionError:  errors.Errorf("uwot"),
			callFwSetEnv:  false,
			ubootVars:     "bootcmd=bootm 20100000",
			want:          nil,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.FileExists = func(filename string) bool {
				return tc.otpCmdExists
			}
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				if cmdArr[0] == "fw_setenv" {
					if tc.callFwSetEnv {
						return 0, nil, "", ""
					}
				} else if cmdArr[0] == "fw_printenv" {
					return 0, nil, tc.ubootVars, ""
				}
				return 0, errors.Errorf("err3"), "", "err3"
			}
			utils.GetOpenBMCVersionFromIssueFile = func() (string, error) {
				return tc.versionString, tc.versionError
			}
			got := enableABR(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
