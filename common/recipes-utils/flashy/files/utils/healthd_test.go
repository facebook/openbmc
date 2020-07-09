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
	"log"
	"os"
	"reflect"
	"testing"

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
