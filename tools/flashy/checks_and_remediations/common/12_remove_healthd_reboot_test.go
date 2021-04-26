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
	"bytes"
	"log"
	"os"
	"testing"
	"time"

	"github.com/Jeffail/gabs"
	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestRemoveHealthdReboot(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore FileExists, ReadFile, WriteFileWithTimeout, RestartHealthd
	fileExistsOrig := fileutils.FileExists
	readFileOrig := fileutils.ReadFile
	writeFileOrig := fileutils.WriteFileWithTimeout

	defer func() {
		log.SetOutput(os.Stderr)
		fileutils.FileExists = fileExistsOrig
		fileutils.ReadFile = readFileOrig
		fileutils.WriteFileWithTimeout = writeFileOrig
	}()

	cases := []struct {
		name              string
		healthdExists     bool
		healthdContents   string
		writeConfigCalled bool
		wantConfig        string
		logContainsSeq    []string
		want              step.StepExitError
	}{
		{
			name:              "basic minilaketb example",
			healthdExists:     true,
			healthdContents:   tests.ExampleMinilaketbHealthdConfigJSON,
			writeConfigCalled: true,
			wantConfig:        tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			logContainsSeq: []string{
				"Healthd exists in this system, removing the high memory utilization " +
					"\"reboot\" entry if it exists.",
			},
			want: nil,
		},
		{
			name:              "basic minilaketb example, reboot already removed",
			healthdExists:     true,
			healthdContents:   tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			writeConfigCalled: false,
			wantConfig:        "",
			logContainsSeq: []string{
				"Healthd exists in this system, removing the high memory utilization " +
					"\"reboot\" entry if it exists.",
			},
			want: nil,
		},
		{
			name:              "Healthd does not exist",
			healthdExists:     false,
			healthdContents:   "",
			writeConfigCalled: false,
			wantConfig:        "",
			logContainsSeq: []string{
				"Healthd does not exist in this system. Skipping step.",
			},
			want: nil,
		},
		{
			name:              "Corrupt healthd-config.json",
			healthdExists:     true,
			healthdContents:   "{ FOOBAR }",
			writeConfigCalled: false,
			wantConfig:        "",
			logContainsSeq:    []string{},
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Unable to parse healthd-config.json: " +
					"invalid character 'F' looking for beginning of object key string"),
			},
		},
		{
			name:              "Error in finding reboot entry",
			healthdExists:     true,
			healthdContents:   "{ \"foo\": \"foo\" }",
			writeConfigCalled: false,
			wantConfig:        "",
			logContainsSeq:    []string{},
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Can't get 'bmc_mem_utilization.threshold' entry in healthd-config " +
					"{\"foo\":\"foo\"}"),
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			writeConfigCalled := false
			fileutils.FileExists = func(filename string) bool {
				return tc.healthdExists
			}
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				return []byte(tc.healthdContents), nil
			}
			fileutils.WriteFileWithTimeout = func(filename string, data []byte, perm os.FileMode, timeout time.Duration) error {
				writeConfigCalled = true
				gotH, err := gabs.ParseJSON(data)
				if err != nil {
					t.Errorf("error parsing edited healthd-config.json")
				}
				// compare with stringified tc.wantConfig
				wantH, err := gabs.ParseJSON([]byte(tc.wantConfig))
				if err != nil {
					t.Errorf("error parsing tc.wantConfig: %v", err)
				}
				if wantH.String() != gotH.String() {
					t.Errorf("resulting healthd-config.json: want '%v' got '%v'",
						wantH.String(), gotH.String())
				}
				return nil
			}

			got := removeHealthdReboot(step.StepParams{})

			step.CompareTestExitErrors(tc.want, got, t)
			if tc.writeConfigCalled != writeConfigCalled {
				t.Errorf("writeConfigCalled: want '%v' got '%v'",
					tc.writeConfigCalled, writeConfigCalled)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
