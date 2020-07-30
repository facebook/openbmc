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
	"log"
	"os"
	"testing"
)

// any other exit codes are programming errors
const (
	unknownErrorExitCode   = 1
	safeToRebootExitCode   = 42
	unsafeToRebootExitCode = 52
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
	return safeToRebootExitCode
}

func (e ExitSafeToReboot) GetType() string {
	return "ExitSafeToReboot"
}

func (e ExitUnsafeToReboot) GetError() string {
	return e.Err.Error()
}

func (e ExitUnsafeToReboot) GetExitCode() int {
	return unsafeToRebootExitCode
}

func (e ExitUnsafeToReboot) GetType() string {
	return "ExitUnsafeToReboot"
}

func (e ExitUnknownError) GetError() string {
	return e.Err.Error()
}

func (e ExitUnknownError) GetExitCode() int {
	return unknownErrorExitCode
}

func (e ExitUnknownError) GetType() string {
	return "ExitUnknownError"
}

func HandleStepError(err StepExitError) {
	// output stack trace
	log.Printf("%+v", err)
	switch err := err.(type) {
	case ExitSafeToReboot,
		ExitUnsafeToReboot,
		ExitUnknownError:

		encodeExitError(err)
		os.Exit(err.GetExitCode())

	default:
		// this should not happen as there are no other types
		log.Printf("Unknown error")
		os.Exit(unknownErrorExitCode)
	}
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
