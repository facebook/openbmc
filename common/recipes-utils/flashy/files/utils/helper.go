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
	"regexp"

	"github.com/pkg/errors"
)

func MaxUint64(x, y uint64) uint64 {
	if x < y {
		return y
	}
	return x
}

// Takes a string slice and looks if a given item is in it. If found it will
// return its index, otherwise it will return -1
func StringFind(val string, arr []string) int {
	for i, item := range arr {
		if item == val {
			return i
		}
	}
	return -1
}

// given a regex with capturing groups and an input string
// return a map of the regex subexpNames to their respective matched
// values
// e.g. regex: "(?P<type>[a-z]+):(?P<specifier>.+)"
//      inputString: "mtd:flash0"
// -> map[string]string { "type": "mtd", "specifier": "flash0" }
// return error if match was not successful
func getRegexSubexpMap(regEx, inputString string) (map[string]string, error) {
	m := make(map[string]string)

	reg, err := regexp.Compile(regEx)
	if err != nil {
		return m, err
	}

	match := reg.FindStringSubmatch(inputString)

	if len(match) == 0 {
		return m, errors.Errorf("No match for regex '%v' for input '%v'", regEx, inputString)
	}

	subexpNames := reg.SubexpNames()

	for i, name := range subexpNames {
		// i > 0 to skip the first empty string returned by
		// FindStringSubmatch
		// i < len(match) for the case of incomplete matches
		if i > 0 && i < len(match) {
			m[name] = match[i]
		}
	}

	return m, nil
}
