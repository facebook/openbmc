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
	"encoding/json"
	"log"
	"os"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// any other exit codes are programming errors
const (
	FLASHY_ERROR_UNKNOWN          = 1
	FLASHY_ERROR_SAFE_TO_REBOOT   = 42
	FLASHY_ERROR_UNSAFE_TO_REBOOT = 52
)

type StepExitError interface {
	GetError() string
	GetExitCode() int
	GetType() string
}

type ExitSafeToReboot struct {
	Err error
}

type ExitUnsafeToReboot struct {
	Err error
}

type ExitUnknownError struct {
	Err error
}

func (e ExitSafeToReboot) GetError() string {
	return e.Err.Error()
}

func (e ExitSafeToReboot) GetExitCode() int {
	return FLASHY_ERROR_SAFE_TO_REBOOT
}

func (e ExitSafeToReboot) GetType() string {
	return "ExitSafeToReboot"
}

func (e ExitUnsafeToReboot) GetError() string {
	return e.Err.Error()
}

func (e ExitUnsafeToReboot) GetExitCode() int {
	return FLASHY_ERROR_UNSAFE_TO_REBOOT
}

func (e ExitUnsafeToReboot) GetType() string {
	return "ExitUnsafeToReboot"
}

func (e ExitUnknownError) GetError() string {
	return e.Err.Error()
}

func (e ExitUnknownError) GetExitCode() int {
	return FLASHY_ERROR_UNKNOWN
}

func (e ExitUnknownError) GetType() string {
	return "ExitUnknownError"
}

var exit = os.Exit

func HandleStepError(err StepExitError) {
	// output stack trace
	log.Printf("%+v", err)
	switch err := err.(type) {
	case ExitSafeToReboot:

		ensureSafeToRebootErr := ensureSafeToReboot()
		if ensureSafeToRebootErr != nil {
			// unsafe to reboot
			// wrap the original error and convert
			log.Printf("UNSAFE TO REBOOT: %v", ensureSafeToRebootErr)
			newErr := ExitUnsafeToReboot{
				errors.Errorf("Original error: %v.\n"+
					"Error converted to Unsafe to Reboot because of: %v",
					err.GetError(), ensureSafeToRebootErr),
			}
			HandleStepError(newErr)
			return
		}

		// actually safe to reboot
		encodeExitError(err)
		exit(err.GetExitCode())

	case ExitUnsafeToReboot,
		ExitUnknownError:

		encodeExitError(err)
		exit(err.GetExitCode())
	default:
		// this should not happen as there are no other types
		log.Printf("Unknown error")
		exit(FLASHY_ERROR_UNKNOWN)
	}
}

// encode exit error
func encodeExitError(err StepExitError) {
	enc := json.NewEncoder(os.Stderr)

	var ae = struct {
		Reason string `json:"message"`
	}{
		Reason: err.GetError(),
	}
	enc.Encode(ae)
}

// ensure that a SafeToReboot Error is actually safe to reboot
// by checking
// (1) no other flashers are running
// TODO:- (2) Either flash0 or flash1 is valid
var ensureSafeToReboot = func() error {
	log.Printf("Ensuring that the system is safe to reboot")

	flashyStepBaseNames := GetFlashyStepBaseNames()
	err := utils.CheckOtherFlasherRunning(flashyStepBaseNames)
	if err != nil {
		return err
	}

	log.Printf("System is safe to reboot.")
	return nil
}

// used to test and compare Exit Errors in testing
func CompareTestExitErrors(want StepExitError, got StepExitError, t *testing.T) {
	if got == nil {
		if want != nil {
			t.Errorf("want '%v' got '%v'", want, got)
		}
	} else {
		if want == nil {
			t.Errorf("want '%v' got '%v'", want, got)
		} else if got.GetError() != want.GetError() || got.GetType() != want.GetType() {
			t.Errorf("want %v:'%v' got %v:'%v'",
				want.GetType(), want.GetError(),
				got.GetType(), got.GetError())
		}
	}
}
