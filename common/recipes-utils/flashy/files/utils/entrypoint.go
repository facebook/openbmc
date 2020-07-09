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
	"log"
	"runtime"
)

type EntryPointMapType = map[string]func(string, string) StepExitError

// maps from the sanitized binary name to the function to run
// the file path keys are also used to symlink the paths (busybox style)
var EntryPointMap = EntryPointMapType{}

// registers endpoints into EntryPointMap
func RegisterStepEntryPoint(step func(string, string) StepExitError) {
	// get the filename 1 stack call above
	_, filename, _, ok := runtime.Caller(1)
	if !ok {
		log.Fatalf("Unable to get filename for step.")
	}

	symlinkPath := getSymlinkPathForSourceFile(filename)
	EntryPointMap[symlinkPath] = step
}
