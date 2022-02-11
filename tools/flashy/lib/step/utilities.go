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
	"runtime"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
)

// UtilitiesFuncType is a function type for utilities functions,
// the argument is a slice of strings representing os.Args.
type UtilitiesFuncType = func([]string) error

// UtilitiesMapType is the type for UtilitiesMap.
type UtilitiesMapType = map[string]UtilitiesFuncType

// UtilitiesMap maps from the sanitized utilities binary name
// to the utility function to run.
var UtilitiesMap = UtilitiesMapType{}

// RegisterUtility registers utility endpoints into UtilitiesMap
func RegisterUtility(f UtilitiesFuncType) {
	// get the filename 1 stack call above
	_, filename, _, ok := runtime.Caller(1)
	if !ok {
		log.Fatalf("Unable to get filename for utility.")
	}

	symlinkPath := fileutils.GetSymlinkPathForSourceFile(filename)
	UtilitiesMap[symlinkPath] = f
}
