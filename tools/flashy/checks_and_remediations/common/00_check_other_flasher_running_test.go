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
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func TestCheckOtherFlasherRunning(t *testing.T) {
	// mock and defer restore CheckOtherFlasherRunning &
	// GetFlashyStepBaseNames
	checkOtherFlasherRunningOrig := utils.CheckOtherFlasherRunning
	getFlashyStepBaseNamesOrig := step.GetFlashyStepBaseNames
	defer func() {
		utils.CheckOtherFlasherRunning = checkOtherFlasherRunningOrig
		step.GetFlashyStepBaseNames = getFlashyStepBaseNamesOrig
	}()

	cases := []struct {
		name                        string
		clowntown                   bool
		checkOtherFlasherRunningErr error
		want                        step.StepExitError
	}{
		{
			name:                        "basic succeeding",
			clowntown:                   false,
			checkOtherFlasherRunningErr: nil,
			want:                        nil,
		},
		{
			name:                        "another flasher running",
			clowntown:                   false,
			checkOtherFlasherRunningErr: errors.Errorf("pypartition"),
			want: step.ExitUnsafeToReboot{
				Err: errors.Errorf("Another flasher detected: %v. "+
					"Use the '--clowntown' flag if you wish to proceed at the risk of "+
					"bricking the device", "pypartition"),
			},
		},
		{
			name:                        "clowntown bypass",
			clowntown:                   true,
			checkOtherFlasherRunningErr: errors.Errorf("pypartition"),
			want:                        nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleFlashyStepBaseNames := []string{"foo", "bar"}

			step.GetFlashyStepBaseNames = func() []string {
				return exampleFlashyStepBaseNames
			}

			utils.CheckOtherFlasherRunning = func(flashyStepBaseNames []string) error {
				if !reflect.DeepEqual(exampleFlashyStepBaseNames, flashyStepBaseNames) {
					t.Errorf("flashyStepBaseNames: want '%v' got '%v'",
						exampleFlashyStepBaseNames, flashyStepBaseNames)
				}
				return tc.checkOtherFlasherRunningErr
			}

			stepParams := step.StepParams{Clowntown: tc.clowntown}
			got := checkOtherFlasherRunning(stepParams)
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
