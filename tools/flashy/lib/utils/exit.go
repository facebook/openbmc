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

package utils

import (
	"encoding/json"
	"log"
	"os"
)

// any other exit codes are programming errors
const (
	unknownErrorExitCode   = 1
	safeToRebootExitCode   = 42
	unsafeToRebootExitCode = 52
)

type StepExitError interface {
	GetError() string
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

func (e ExitUnsafeToReboot) GetError() string {
	return e.Err.Error()
}

func (e ExitUnknownError) GetError() string {
	return e.Err.Error()
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

func HandleError(err StepExitError) {
	// output stack trace
	log.Printf("%+v", err)
	switch err := err.(type) {
	case ExitSafeToReboot:
		encodeExitError(err)
		os.Exit(safeToRebootExitCode)

	case ExitUnsafeToReboot:
		encodeExitError(err)
		os.Exit(unsafeToRebootExitCode)

	case ExitUnknownError:
		encodeExitError(err)
		os.Exit(unknownErrorExitCode)

	default:
		// this should not happen as there are no other types
		log.Printf("Unknown error")
		os.Exit(unknownErrorExitCode)
	}
}
