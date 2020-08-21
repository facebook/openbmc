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

package remediations_wedge100

import (
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func TestFixROMCS1(t *testing.T) {
	// mock and defer restore RunCommand
	runCommandOrig := utils.RunCommand
	defer func() {
		utils.RunCommand = runCommandOrig
	}()
	cases := []struct {
		name      string
		runCmdErr error
		want      step.StepExitError
	}{
		{
			name:      "succeeded",
			runCmdErr: nil,
			want:      nil,
		},
		{
			name:      "run command failed",
			runCmdErr: errors.Errorf("command failed"),
			want: step.ExitSafeToReboot{
				errors.Errorf("Failed to run ROMCS1# fix: command failed"),
			},
		},
	}

	wantCmd := "/usr/local/bin/openbmc_gpio_util.py config ROMCS1#"
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if wantCmd != gotCmd {
					t.Errorf("cmd: want '%v' got '%v'", wantCmd, gotCmd)
				}
				return 0, tc.runCmdErr, "", ""
			}
			got := fixROMCS1(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
