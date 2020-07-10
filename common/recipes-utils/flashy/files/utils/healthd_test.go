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
	"bytes"
	"fmt"
	"log"
	"os"
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tree/helium/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

var exampleMinilaketbHealthdConfig = HealthdConfig{
	BmcMemUtil{
		[]BmcMemThres{
			BmcMemThres{
				Value:   float32(60.0),
				Actions: []string{"log-warning"},
			},
			BmcMemThres{
				Value:   float32(70.0),
				Actions: []string{"log-critical", "bmc-error-trigger"},
			},
			BmcMemThres{
				Value:   float32(95.0),
				Actions: []string{"log-critical", "bmc-error-trigger", "reboot"},
			},
		},
	},
}

var exampleNoRebootHealthdConfig = HealthdConfig{
	BmcMemUtil{
		[]BmcMemThres{
			BmcMemThres{
				Value:   float32(60.0),
				Actions: []string{"log-warning"},
			},
			BmcMemThres{
				Value:   float32(70.0),
				Actions: []string{"log-critical", "bmc-error-trigger"},
			},
		},
	},
}

func TestGetHealthdConfig(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := ReadFile
	healthdExistsOrig := HealthdExists
	defer func() {
		ReadFile = readFileOrig
		HealthdExists = healthdExistsOrig
	}()

	cases := []struct {
		name             string
		healthdExists    bool
		readFileContents string
		readFileErr      error
		want             *HealthdConfig
		wantErr          error
	}{
		{
			name:             "Minilaketb example healthd-config.json",
			healthdExists:    true,
			readFileContents: tests.ExampleMinilaketbHealthdConfigJSON,
			readFileErr:      nil,
			want:             &exampleMinilaketbHealthdConfig,
			wantErr:          nil,
		},
		{
			name:             "Minimal example healthd-config.json",
			healthdExists:    true,
			readFileContents: tests.ExampleMinilaketbHealthdConfigJSON,
			readFileErr:      nil,
			want:             &exampleMinilaketbHealthdConfig,
			wantErr:          nil,
		},
		{
			name:             "Empty healthd config",
			healthdExists:    true,
			readFileContents: "",
			readFileErr:      nil,
			want:             nil,
			wantErr:          errors.Errorf("Unable to unmarshal healthd-config.json: unexpected end of JSON input"),
		},
		{
			name:             "Corrupt healthd config",
			healthdExists:    true,
			readFileContents: tests.ExampleCorruptedHealthdConfig,
			readFileErr:      nil,
			want:             nil,
			wantErr:          errors.Errorf("Unable to unmarshal healthd-config.json: invalid character '\\n' in string literal"),
		},
		{
			name:             "Readfile error",
			healthdExists:    true,
			readFileContents: "",
			readFileErr:      errors.Errorf("ReadFile error"),
			want:             nil,
			wantErr:          errors.Errorf("Unable to open healthd config: ReadFile error"),
		},
		{
			name:             "File does not exist",
			healthdExists:    false,
			readFileContents: "",
			readFileErr:      nil,
			want:             nil,
			wantErr:          errors.Errorf("Unable to find healthd-config.json file"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			ReadFile = func(filename string) ([]byte, error) {
				if filename != healthdConfigFilePath {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", healthdConfigFilePath, filename)
				}
				return []byte(tc.readFileContents), tc.readFileErr
			}
			HealthdExists = func() bool {
				return tc.healthdExists
			}
			got, err := GetHealthdConfig()

			tests.CompareTestErrors(tc.wantErr, err, t)

			if got == nil {
				if tc.want != nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				}
			} else {
				if tc.want == nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				} else if !reflect.DeepEqual(*got, *tc.want) {
					t.Errorf("want '%v' got '%v'", *tc.want, *got)
				}
			}

		})
	}
}

func TestGetRebootThresholdPercentage(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	defer func() {
		log.SetOutput(os.Stderr)
	}()

	cases := []struct {
		name           string
		healthdConfig  *HealthdConfig
		logContainsSeq []string
		want           float32
	}{
		{
			name:          "example minilaketb healthd config",
			healthdConfig: &exampleMinilaketbHealthdConfig,
			logContainsSeq: []string{
				"Found healthd reboot threshold: 95 percent",
			},
			want: float32(95),
		},
		{
			name:          "example no reboot healthd config",
			healthdConfig: &exampleNoRebootHealthdConfig,
			logContainsSeq: []string{
				"No healthd reboot threshold found, defaulting to 100 percent",
			},
			want: float32(100),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}

			got := tc.healthdConfig.GetRebootThresholdPercentage()

			if got != tc.want {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestRestartHealthd(t *testing.T) {
	// save and defer restore FileExists and RunCommand
	fileExistsOrig := FileExists
	runCommandOrig := RunCommand
	defer func() {
		FileExists = fileExistsOrig
		RunCommand = runCommandOrig
	}()

	cases := []struct {
		name          string
		wait          bool
		supervisor    string
		fileExists    bool
		runCmdErr     error
		want          error
		wantSleepTime time.Duration
	}{
		{
			name:          "Normal restart operation with sv, wait",
			wait:          true,
			supervisor:    "sv",
			fileExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 30 * time.Second,
		},
		{
			name:          "Normal restart operation, no wait",
			wait:          false,
			supervisor:    "sv",
			fileExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 0 * time.Second,
		},
		{
			name:          "Normal restart operation with systemctl, wait",
			wait:          true,
			supervisor:    "systemctl",
			fileExists:    true,
			runCmdErr:     nil,
			want:          nil,
			wantSleepTime: 30 * time.Second,
		},
		{
			name:          "/etc/sv/healthd does not exist",
			wait:          true,
			supervisor:    "sv",
			fileExists:    false,
			runCmdErr:     nil,
			want:          errors.Errorf("Error restarting healthd: '/etc/sv/healthd' does not exist"),
			wantSleepTime: 0 * time.Second,
		},
		{
			name:          "Restart command returned error",
			wait:          true,
			supervisor:    "systemctl",
			fileExists:    true,
			runCmdErr:     errors.Errorf("RunCommand error"),
			want:          errors.Errorf("RunCommand error"),
			wantSleepTime: 0 * time.Second,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			var gotSleepTime time.Duration
			var sleepFunc = func(t time.Duration) {
				gotSleepTime = t
			}
			FileExists = func(filename string) bool {
				if filename != "/etc/sv/healthd" {
					t.Errorf("filename: want %v got %v", "/etc/sv/healthd", filename)
				}
				return tc.fileExists
			}
			RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
				wantCmd := fmt.Sprintf("%v restart healthd", tc.supervisor)
				gotCmd := strings.Join(cmdArr, " ")
				if wantCmd != gotCmd {
					t.Errorf("command: want '%v' got '%v'", wantCmd, gotCmd)
				}
				if timeoutInSeconds != 60 {
					t.Errorf("timeout: want 60 got %v", timeoutInSeconds)
				}
				// exit code ignored
				return 0, tc.runCmdErr, "", ""
			}
			got := RestartHealthd(tc.wait, tc.supervisor, sleepFunc)

			tests.CompareTestErrors(tc.want, got, t)
			if gotSleepTime != tc.wantSleepTime {
				t.Errorf("sleeptime: want '%v' got '%v'", tc.wantSleepTime, gotSleepTime)
			}
		})
	}
}
