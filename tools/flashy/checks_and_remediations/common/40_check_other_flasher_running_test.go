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

package common

import (
	"fmt"
	"os"
	"reflect"
	"sort"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestCheckOtherFlasherRunning(t *testing.T) {
	// mock and defer restore getFlashyStepBaseNames,
	// getOtherCmdlines and checkNoBaseNameExistsInCmdlines
	getFlashyStepBaseNamesOrig := getFlashyStepBaseNames
	getOtherProcCmdlinePathsOrig := getOtherProcCmdlinePaths
	checkNoBaseNameExistsInProcCmdlinePathsOrig := checkNoBaseNameExistsInProcCmdlinePaths
	defer func() {
		getFlashyStepBaseNames = getFlashyStepBaseNamesOrig
		getOtherProcCmdlinePaths = getOtherProcCmdlinePathsOrig
		checkNoBaseNameExistsInProcCmdlinePaths = checkNoBaseNameExistsInProcCmdlinePathsOrig
	}()

	cases := []struct {
		name                     string
		clowntown                bool
		otherCmdlinesRet         []string
		otherCmdLinesErr         error
		checkNoBaseNameExistsErr error
		want                     step.StepExitError
	}{
		{
			name:      "basic succeeding",
			clowntown: false,
			otherCmdlinesRet: []string{
				"/proc/42/cmdline",
			},
			otherCmdLinesErr:         nil,
			checkNoBaseNameExistsErr: nil,
			want:                     nil,
		},
		{
			name:                     "getOtherCmdlines error",
			clowntown:                false,
			otherCmdlinesRet:         nil,
			otherCmdLinesErr:         errors.Errorf("getOtherCmdlines error"),
			checkNoBaseNameExistsErr: nil,
			want: step.ExitUnknownError{
				errors.Errorf("getOtherCmdlines error"),
			},
		},
		{
			name:                     "baseName found in cmdline",
			clowntown:                false,
			otherCmdlinesRet:         nil,
			otherCmdLinesErr:         nil,
			checkNoBaseNameExistsErr: errors.Errorf("Found another flasher error"),
			want: step.ExitUnsafeToReboot{
				errors.Errorf("Another flasher detected: %v. "+
					"Use the '--clowntown' flag if you wish to proceed at the risk of "+
					"bricking the device", errors.Errorf("Found another flasher error")),
			},
		},
		{
			name:                     "clowntown ignore error",
			clowntown:                true,
			otherCmdlinesRet:         nil,
			otherCmdLinesErr:         errors.Errorf("getOtherCmdlines error"),
			checkNoBaseNameExistsErr: errors.Errorf("Found another flasher error"),
			want:                     nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			getFlashyStepBaseNames = func() []string {
				return []string{
					"checks_and_remediations/common/00_dummy_step",
				}
			}
			getOtherProcCmdlinePaths = func() ([]string, error) {
				return tc.otherCmdlinesRet, tc.otherCmdLinesErr
			}
			checkNoBaseNameExistsInProcCmdlinePaths = func(baseNames, cmdlines []string) error {
				return tc.checkNoBaseNameExistsErr
			}

			stepParams := step.StepParams{Clowntown: tc.clowntown}
			got := checkOtherFlasherRunning(stepParams)
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}

func TestCheckNoBaseNameExistsInProcCmdlinePaths(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	type ReadFileRetType struct {
		Buf []byte
		Err error
	}

	baseNames := []string{
		"improve_system.py",
		"fw-util",
		"flashy",
		"00_dummy_step",
	}

	cases := []struct {
		name string
		// map from file name to the return
		// the keys of this map are cmdlines
		readFileRet map[string]interface{}
		want        error
	}{
		{
			name: "no basename exists in cmdlines",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline":  ReadFileRetType{[]byte("python\x00test.py"), nil},
				"/proc/420/cmdline": ReadFileRetType{[]byte("ls\x00-la"), nil},
			},
			want: nil,
		},
		{
			name: "ignore readfile errors",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline":  ReadFileRetType{[]byte{}, errors.Errorf("!")},
				"/proc/420/cmdline": ReadFileRetType{[]byte{}, errors.Errorf("!")},
			},
			want: nil,
		},
		{
			name: "base name exists (1)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte("python\x00/tmp/improve_system.py"), nil},
			},
			want: errors.Errorf("'improve_system.py' found in cmdline 'python /tmp/improve_system.py'"),
		},
		{
			name: "base name exists (2)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte("fw-util\x00-foobar"), nil},
			},
			want: errors.Errorf("'fw-util' found in cmdline 'fw-util -foobar'"),
		},
		{
			name: "base name exists (3)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte(
					"/opt/flashy/checks_and_remediations/common/00_dummy_step\x00-device\x00mtd:flash0",
				), nil},
			},
			want: errors.Errorf("'00_dummy_step' found in cmdline " +
				"'/opt/flashy/checks_and_remediations/common/00_dummy_step -device mtd:flash0'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				ret := tc.readFileRet[filename].(ReadFileRetType)
				return ret.Buf, ret.Err
			}
			cmdlines := utils.GetStringKeys(tc.readFileRet)
			got := checkNoBaseNameExistsInProcCmdlinePaths(baseNames, cmdlines)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestGetFlashyStepBaseNames(t *testing.T) {
	// mock and defer restore StepMap
	stepMapOrig := step.StepMap
	defer func() {
		step.StepMap = stepMapOrig
	}()

	var dummyStep = func(step.StepParams) step.StepExitError {
		return nil
	}

	step.StepMap = step.StepMapType{
		"checks_and_remediations/common/00_truncate_logs": dummyStep,
		"checks_and_remediations/wedge100/00_fix_romcs1":  dummyStep,
		"flash_procedure/flash_wedge100":                  dummyStep,
	}
	want := []string{
		"00_truncate_logs",
		"00_fix_romcs1",
		"flash_wedge100",
	}
	got := getFlashyStepBaseNames()
	// order is not guaranteed in a map, so we sort first
	sort.Strings(want)
	sort.Strings(got)

	if !reflect.DeepEqual(want, got) {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}

func TestGetOtherProcCmdlinePaths(t *testing.T) {
	// mock and defer restore Glob and ownCmdlines
	globOrig := fileutils.Glob
	ownCmdlinesOrig := ownCmdlines
	defer func() {
		fileutils.Glob = globOrig
		ownCmdlines = ownCmdlinesOrig
	}()
	cases := []struct {
		name    string
		globRet []string
		globErr error
		want    []string
		wantErr error
	}{
		{
			name: "normal operation",
			globRet: []string{
				"foo",
				"bar",
				"/proc/self/cmdline",
				"/proc/thread-self/cmdline",
			},
			globErr: nil,
			want:    []string{"foo", "bar"},
			wantErr: nil,
		},
		{
			name:    "glob error",
			globRet: nil,
			globErr: errors.Errorf("glob error"),
			want:    nil,
			wantErr: errors.Errorf("glob error"),
		},
	}
	ownCmdlines = []string{
		"/proc/self/cmdline",
		"/proc/thread-self/cmdline",
		fmt.Sprintf("/proc/%v/cmdline", os.Getpid()),
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.Glob = func(pattern string) (matches []string, err error) {
				return tc.globRet, tc.globErr
			}
			got, err := getOtherProcCmdlinePaths()
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}
