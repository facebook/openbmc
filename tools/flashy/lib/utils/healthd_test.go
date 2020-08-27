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
	"fmt"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/Jeffail/gabs"
	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestGetHealthdConfig(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name             string
		readFileContents string
		readFileErr      error
		wantErr          error
	}{
		{
			name:             "Minilaketb example healthd-config.json",
			readFileContents: tests.ExampleMinilaketbHealthdConfigJSON,
			readFileErr:      nil,
			wantErr:          nil,
		},
		{
			name:             "Minimal example healthd-config.json",
			readFileContents: tests.ExampleMinimalHealthdConfigJSON,
			readFileErr:      nil,
			wantErr:          nil,
		},
		{
			name:             "Empty healthd config",
			readFileContents: "",
			readFileErr:      nil,
			wantErr:          errors.Errorf("Unable to parse healthd-config.json: unexpected end of JSON input"),
		},
		{
			name:             "Corrupt healthd config",
			readFileContents: "",
			readFileErr:      nil,
			wantErr:          errors.Errorf("Unable to parse healthd-config.json: unexpected end of JSON input"),
		},
		{
			name:             "Readfile error",
			readFileContents: "",
			readFileErr:      errors.Errorf("ReadFile error"),
			wantErr:          errors.Errorf("Unable to open healthd-config.json: ReadFile error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != healthdConfigFilePath {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", healthdConfigFilePath, filename)
				}
				return []byte(tc.readFileContents), tc.readFileErr
			}
			_, err := GetHealthdConfig()

			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestHealthdRemoveMemUtilRebootEntryIfExists(t *testing.T) {
	// save and defer restore WriteFileWithTimeout and RestartHealthd
	writeFileOrig := fileutils.WriteFileWithTimeout
	restartHealthdOrig := RestartHealthd
	defer func() {
		fileutils.WriteFileWithTimeout = writeFileOrig
		RestartHealthd = restartHealthdOrig
	}()

	cases := []struct {
		name              string
		inputJSON         string
		wantJSON          string
		writeConfigCalled bool
		writeConfigErr    error
		restartHealthdErr error
		wantErr           error
	}{
		{
			name:              "basic minimal success",
			inputJSON:         tests.ExampleMinimalHealthdConfigJSON,
			wantJSON:          tests.ExampleMinimalHealthdConfigJSONRemovedReboot,
			writeConfigCalled: true,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr:           nil,
		},
		{
			name:              "minilaketb example",
			inputJSON:         tests.ExampleMinilaketbHealthdConfigJSON,
			wantJSON:          tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			writeConfigCalled: true,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr:           nil,
		},
		{
			name:              "minilaketb reboot already removed",
			inputJSON:         tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			wantJSON:          tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			writeConfigCalled: false,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr:           nil,
		},
		{
			name:              "no bmc_mem_utilization entry in Json",
			inputJSON:         "{}",
			wantJSON:          "",
			writeConfigCalled: false,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr:           errors.Errorf("Can't get 'bmc_mem_utilization.threshold' entry in healthd-config {}"),
		},
		{
			name: "invalid type for bmc_mem_utilization.threshold (not array)",
			inputJSON: `{
	"bmc_mem_utilization": {
		"threshold": 42
	}
}`,
			wantJSON:          "",
			writeConfigCalled: false,
			writeConfigErr:    nil,
			wantErr: errors.Errorf("Can't get 'bmc_mem_utilization.threshold' entry in healthd-config " +
				`{"bmc_mem_utilization":{"threshold":42}}`),
		},
		{
			name: "invalid type for bmc_mem_utilization.threshold[x].action (not interface array)",
			inputJSON: `{
	"bmc_mem_utilization": {
		"threshold": [
			{
				"action": 42
			}
		]
	}
}`,
			wantJSON:          "",
			writeConfigCalled: false,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr: errors.Errorf("Can't get 'action' entry in healthd-config " +
				"{\"bmc_mem_utilization\":{\"threshold\":[{\"action\":42}]}}"),
		},
		{
			name:              "restart healthd failure",
			inputJSON:         tests.ExampleMinilaketbHealthdConfigJSON,
			wantJSON:          tests.ExampleMinilaketbHealthdConfigJSONRemovedReboot,
			writeConfigCalled: true,
			writeConfigErr:    nil,
			restartHealthdErr: errors.Errorf("Restart healthd failed"),
			wantErr:           errors.Errorf("Restart healthd failed"),
		},
		{
			name:              "remove multiple reboot entries",
			inputJSON:         tests.ExampleMinimalHealthdConfigJSONMultipleReboots,
			wantJSON:          tests.ExampleMinimalHealthdConfigJSONRemovedReboot,
			writeConfigCalled: true,
			writeConfigErr:    nil,
			restartHealthdErr: nil,
			wantErr:           nil,
		},
		{
			name:              "write config failed",
			inputJSON:         tests.ExampleMinimalHealthdConfigJSON,
			wantJSON:          tests.ExampleMinimalHealthdConfigJSONRemovedReboot,
			writeConfigCalled: true,
			writeConfigErr:    errors.Errorf("Write config failed"),
			restartHealthdErr: nil,
			wantErr:           errors.Errorf("Unable to write healthd config to file: Write config failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			writeConfigCalled := false
			fileutils.WriteFileWithTimeout = func(filename string, data []byte, perm os.FileMode, timeout time.Duration) error {
				writeConfigCalled = true
				return tc.writeConfigErr
			}
			RestartHealthd = func(wait bool, supervisor string) error {
				return tc.restartHealthdErr
			}
			h, err := gabs.ParseJSON([]byte(tc.inputJSON))
			if err != nil {
				t.Errorf("%v", err)
			}
			err = HealthdRemoveMemUtilRebootEntryIfExists(h)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if writeConfigCalled != tc.writeConfigCalled {
				t.Errorf("writeConfigCalled: want '%v' got '%v'",
					tc.writeConfigCalled, writeConfigCalled)
			}
			if len(tc.wantJSON) > 0 {
				wantH, err := gabs.ParseJSON([]byte(tc.wantJSON))
				if err != nil {
					t.Errorf("%v", err)
				}
				// compare the stringified versions
				if wantH.String() != h.String() {
					t.Errorf("want '%v' got '%v'", wantH.String(), h.String())
				}
			}
		})
	}
}

func TestHealthdWriteConfigToFile(t *testing.T) {
	// save and defer restore WriteFileWithTimeout
	writeFileOrig := fileutils.WriteFileWithTimeout
	defer func() {
		fileutils.WriteFileWithTimeout = writeFileOrig
	}()

	cases := []struct {
		name         string
		writeFileErr error
		want         error
	}{
		{
			name:         "success",
			writeFileErr: nil,
			want:         nil,
		},
		{
			name:         "WriteFile error",
			writeFileErr: errors.Errorf("WriteFile err"),
			want:         errors.Errorf("Unable to write healthd config to file: WriteFile err"),
		},
	}

	mockGabsContainer := gabs.Container{}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.WriteFileWithTimeout = func(filename string, data []byte, perm os.FileMode, timeout time.Duration) error {
				return tc.writeFileErr
			}
			got := HealthdWriteConfigToFile(&mockGabsContainer)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestRestartHealthd(t *testing.T) {
	// save and defer restore FileExists, RunCommand & Sleep
	pathExistsOrig := fileutils.PathExists
	runCommandOrig := RunCommand
	sleepFuncOrig := Sleep
	defer func() {
		fileutils.PathExists = pathExistsOrig
		RunCommand = runCommandOrig
		Sleep = sleepFuncOrig
	}()

	cases := []struct {
		name          string
		wait          bool
		supervisor    string
		pathExists    bool
		runCmdErr     error
		want          error
		wantSleepTime time.Duration
	}{
		{
			name:          "Normal restart operation with sv, wait",
			wait:          true,
			supervisor:    "sv",
			pathExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 30 * time.Second,
		},
		{
			name:          "Normal restart operation, no wait",
			wait:          false,
			supervisor:    "sv",
			pathExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 0 * time.Second,
		},
		{
			name:          "Normal restart operation with systemctl, wait",
			wait:          true,
			supervisor:    "systemctl",
			pathExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 30 * time.Second,
		},
		{
			name:          "/etc/sv/healthd does not exist",
			wait:          true,
			supervisor:    "sv",
			pathExists:    false,
			runCmdErr:     nil,
			want:          errors.Errorf("Error restarting healthd: '/etc/sv/healthd' does not exist"),
			wantSleepTime: 0 * time.Second,
		},
		{
			name:          "Restart command returned error",
			wait:          true,
			supervisor:    "systemctl",
			pathExists:    true,
			runCmdErr:     errors.Errorf("RunCommand error"),
			want:          errors.Errorf("RunCommand error"),
			wantSleepTime: 0 * time.Second,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var gotSleepTime time.Duration
			Sleep = func(t time.Duration) {
				gotSleepTime = t
			}
			fileutils.PathExists = func(filename string) bool {
				if filename != "/etc/sv/healthd" {
					t.Errorf("filename: want %v got %v", "/etc/sv/healthd", filename)
				}
				return tc.pathExists
			}
			RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				wantCmd := fmt.Sprintf("%v restart healthd", tc.supervisor)
				gotCmd := strings.Join(cmdArr, " ")
				if wantCmd != gotCmd {
					t.Errorf("command: want '%v' got '%v'", wantCmd, gotCmd)
				}
				// exit code ignored
				return 0, tc.runCmdErr, "", ""
			}
			got := RestartHealthd(tc.wait, tc.supervisor)

			tests.CompareTestErrors(tc.want, got, t)
			if gotSleepTime != tc.wantSleepTime {
				t.Errorf("sleeptime: want '%v' got '%v'", tc.wantSleepTime, gotSleepTime)
			}
		})
	}
}
