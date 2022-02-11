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
	"path"
	"runtime"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
)

// StepParams contains arguments passed into steps.
type StepParams struct {
	ImageFilePath string
	DeviceID      string
	Clowntown     bool
}

// StepMapType is the type for StepMap.
type StepMapType = map[string]func(StepParams) StepExitError

// StepMap maps from the sanitized binary name to the function to run
// the file path keys are also used to symlink the paths (busybox style).
var StepMap = StepMapType{}

// RegisterStep registers endpoints into StepMap.
func RegisterStep(step func(StepParams) StepExitError) {
	// get the filename 1 stack call above
	_, filename, _, ok := runtime.Caller(1)
	if !ok {
		log.Fatalf("Unable to get filename for step.")
	}

	symlinkPath := fileutils.GetSymlinkPathForSourceFile(filename)
	StepMap[symlinkPath] = step
}

// GetFlashyStepBaseNames gets basenames of flashy's steps.
var GetFlashyStepBaseNames = func() []string {
	flashyStepBaseNames := []string{}
	for p := range StepMap {
		stepBasename := path.Base(p)
		flashyStepBaseNames = append(flashyStepBaseNames, stepBasename)
	}
	return flashyStepBaseNames
}
