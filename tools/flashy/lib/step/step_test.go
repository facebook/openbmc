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
	"sort"
	"testing"
)

func TestGetFlashyStepBaseNames(t *testing.T) {
	// mock and defer restore StepMap
	stepMapOrig := StepMap
	defer func() {
		StepMap = stepMapOrig
	}()

	var dummyStep = func(StepParams) StepExitError {
		return nil
	}

	StepMap = StepMapType{
		"checks_and_remediations/common/00_truncate_logs": dummyStep,
		"checks_and_remediations/wedge100/00_fix_romcs1":  dummyStep,
		"flash_procedure/flash_wedge100":                  dummyStep,
	}
	want := []string{
		"00_truncate_logs",
		"00_fix_romcs1",
		"flash_wedge100",
	}
	got := GetFlashyStepBaseNames()
	// order is not guaranteed in a map, so we sort first
	sort.Strings(want)
	sort.Strings(got)

	if !reflect.DeepEqual(want, got) {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}
