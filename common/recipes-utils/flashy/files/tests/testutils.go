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

package tests

import (
	"strings"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
)

// used to test and compare errors in testing
func CompareTestErrors(want error, got error, t *testing.T) {
	if got == nil {
		if want != nil {
			t.Errorf("want '%v' got '%v'", want, got)
		}
	} else {
		if want == nil {
			t.Errorf("want '%v' got '%v'", want, got)
		} else if got.Error() != want.Error() {
			t.Errorf("want '%v' got '%v'", want.Error(), got.Error())
		}
	}
}

// used to test and compare Exit Errors in testing
func CompareTestExitErrors(want utils.StepExitError, got utils.StepExitError, t *testing.T) {
	if got == nil {
		if want != nil {
			t.Errorf("want '%v' got '%v'", want, got)
		}
	} else {
		if want == nil {
			t.Errorf("want '%v' got '%v'", want, got)
		} else if got.GetError() != want.GetError() {
			t.Errorf("want '%v' got '%v'", want.GetError(), got.GetError())
		}
	}
}

// check that logs are complete and contain all
// strings in logContainsSeq in sequence
func LogContainsSeqTest(logs string, logContainsSeq []string, t *testing.T) {
	lastIndex := -1

	for _, s := range logContainsSeq {
		idx := strings.Index(logs, s)
		if idx == -1 {
			t.Errorf("Logs incomplete, want '%v' got '%v'", s, logs)
		} else if idx < lastIndex {
			t.Errorf("Log sequence wrong, want sequence %#v got %v", logContainsSeq, logs)
		}
		lastIndex = idx
	}
}

// return true if the filename denotes a `_test.go` file
func IsGoTestFileName(filename string) bool {
	return len(filename) >= 8 &&
		filename[len(filename)-8:] == "_test.go"
}
