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
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestRestartServices(t *testing.T) {
	// save and defer restore SystemdAvailable, RunCommand, RestartHealtd
	systemdAvailableOrig := utils.SystemdAvailable
	runCommandOrig := utils.RunCommand
	restartHealthdOrig := utils.RestartHealthd
	healthdExistsOrig := utils.HealthdExists
	defer func() {
		utils.SystemdAvailable = systemdAvailableOrig
		utils.RunCommand = runCommandOrig
		utils.RestartHealthd = restartHealthdOrig
		utils.HealthdExists = healthdExistsOrig
	}()

	cases := []struct {
		name              string
		systemdAvail      bool
		systemdAvailErr   error
		runCmdErr         error
		restartHealthdErr error
		want              utils.StepExitError
		wantCmds          []string // check RunCommand ran all wanted commands
	}{
		{
			name:              "Normal operation, systemd available",
			systemdAvail:      true,
			systemdAvailErr:   nil,
			runCmdErr:         nil,
			restartHealthdErr: nil,
			want:              nil,
			wantCmds:          []string{"systemctl restart restapi"},
		},
		{
			name:              "Normal operation, systemd not available",
			systemdAvail:      false,
			systemdAvailErr:   nil,
			runCmdErr:         nil,
			restartHealthdErr: nil,
			want:              nil,
			wantCmds:          []string{"sv restart restapi"},
		},
		{
			name:              "systemd check error",
			systemdAvail:      false,
			systemdAvailErr:   errors.Errorf("Systemd check error"),
			runCmdErr:         nil,
			restartHealthdErr: nil,
			want:              utils.ExitSafeToReboot{errors.Errorf("Error checking systemd availability: Systemd check error")},
			wantCmds:          []string{},
		},
		{
			name:              "Restapi restart error",
			systemdAvail:      true,
			systemdAvailErr:   nil,
			runCmdErr:         errors.Errorf("Restapi restart error"),
			restartHealthdErr: nil,
			want:              utils.ExitSafeToReboot{errors.Errorf("Could not restart restapi: Restapi restart error")},
			wantCmds:          []string{"systemctl restart restapi"},
		},
		{
			name:              "Healthd restart error",
			systemdAvail:      true,
			systemdAvailErr:   nil,
			runCmdErr:         nil,
			restartHealthdErr: errors.Errorf("Healthd restart error"),
			want:              utils.ExitSafeToReboot{errors.Errorf("Could not restart healthd: Healthd restart error")},
			wantCmds:          []string{"systemctl restart restapi"},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var gotCmds = []string{}
			utils.RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
				gotCmds = append(gotCmds, strings.Join(cmdArr, " "))
				// does not matter what exit code is given here, as only err is checked
				return 0, tc.runCmdErr, "", ""
			}
			utils.SystemdAvailable = func() (bool, error) {
				return tc.systemdAvail, tc.systemdAvailErr
			}
			utils.RestartHealthd = func(wait bool, supervisor string, sleepFunc func(time.Duration)) error {
				return tc.restartHealthdErr
			}
			utils.HealthdExists = func() bool {
				// always assume healthd exists
				return true
			}

			got := restartServices("x", "x")

			tests.CompareTestExitErrors(tc.want, got, t)

			if !reflect.DeepEqual(gotCmds, tc.wantCmds) {
				t.Errorf("commands: want '%#v' got '%#v'", tc.wantCmds, gotCmds)
			}
		})
	}
}
