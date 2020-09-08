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

package step

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestHandleStepError(t *testing.T) {
	// mock and defer restore exit & ensureSafeToReboot
	exitOrig := exit
	ensureSafeToRebootOrig := ensureSafeToReboot
	defer func() {
		exit = exitOrig
		ensureSafeToReboot = ensureSafeToRebootOrig
	}()

	cases := []struct {
		name                  string
		stepExitError         StepExitError
		ensureSafeToRebootErr error
		wantExitCode          int
	}{
		{
			name:                  "unknown error",
			stepExitError:         ExitUnknownError{errors.Errorf("err")},
			ensureSafeToRebootErr: nil,
			wantExitCode:          FLASHY_ERROR_UNKNOWN,
		},
		{
			name:                  "unsafe to reboot",
			stepExitError:         ExitUnsafeToReboot{errors.Errorf("err")},
			ensureSafeToRebootErr: nil,
			wantExitCode:          FLASHY_ERROR_UNSAFE_TO_REBOOT,
		},
		{
			name:                  "safe to reboot, ensured OK",
			stepExitError:         ExitSafeToReboot{errors.Errorf("err")},
			ensureSafeToRebootErr: nil,
			wantExitCode:          FLASHY_ERROR_SAFE_TO_REBOOT,
		},
		{
			name:                  "safe to reboot, but ensure check failed",
			stepExitError:         ExitSafeToReboot{errors.Errorf("err")},
			ensureSafeToRebootErr: errors.Errorf("actually unsafe"),
			wantExitCode:          FLASHY_ERROR_UNSAFE_TO_REBOOT,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			ensureSafeToReboot = func() error {
				return tc.ensureSafeToRebootErr
			}
			exitCalled := false
			exit = func(code int) {
				exitCalled = true
				if code != tc.wantExitCode {
					t.Errorf("want '%v' got '%v'", tc.wantExitCode, code)
				}
			}

			HandleStepError(tc.stepExitError)

			if !exitCalled {
				t.Errorf("exit not called")
			}
		})
	}
}

func TestEnsureSafeToReboot(t *testing.T) {
	// mock and defer restore CheckOtherFlasherRunning,
	// GetFlashyStepBaseNames and CheckAnyFlashDeviceValid
	getFlashyStepBaseNamesOrig := GetFlashyStepBaseNames
	checkOtherFlasherRunningOrig := utils.CheckOtherFlasherRunning
	checkAnyFlashDeviceValidOrig := flashutils.CheckAnyFlashDeviceValid
	defer func() {
		GetFlashyStepBaseNames = getFlashyStepBaseNamesOrig
		utils.CheckOtherFlasherRunning = checkOtherFlasherRunningOrig
		flashutils.CheckAnyFlashDeviceValid = checkAnyFlashDeviceValidOrig
	}()

	cases := []struct {
		name                     string
		otherFlasherRunningErr   error
		checkFlashDeviceValidErr error
		want                     error
	}{
		{
			name:                     "all checks passed",
			otherFlasherRunningErr:   nil,
			checkFlashDeviceValidErr: nil,
			want:                     nil,
		},
		{
			name:                     "other flasher running",
			otherFlasherRunningErr:   errors.Errorf("Found other flasher running"),
			checkFlashDeviceValidErr: nil,
			want:                     errors.Errorf("Found other flasher running"),
		},
		{
			name:                     "no flash device valid",
			otherFlasherRunningErr:   nil,
			checkFlashDeviceValidErr: errors.Errorf("No flash device is valid"),
			want:                     errors.Errorf("No flash device is valid"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleFlashyStepBaseNames := []string{"foo", "bar"}
			GetFlashyStepBaseNames = func() []string {
				return exampleFlashyStepBaseNames
			}
			utils.CheckOtherFlasherRunning = func(flashyStepBaseNames []string) error {
				if !reflect.DeepEqual(exampleFlashyStepBaseNames, flashyStepBaseNames) {
					t.Errorf("flashyStepBaseNames: want '%v' got '%v'",
						exampleFlashyStepBaseNames, flashyStepBaseNames)
				}
				return tc.otherFlasherRunningErr
			}
			flashutils.CheckAnyFlashDeviceValid = func() error {
				return tc.checkFlashDeviceValidErr
			}

			got := ensureSafeToReboot()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
