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
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tree/helium/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestGetMemInfo(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := ReadFile
	defer func() { ReadFile = readFileOrig }()

	cases := []struct {
		name             string
		readFileContents string
		readFileErr      error
		want             *MemInfo
		wantErr          error
	}{
		{
			name:             "wedge100 example MemInfo",
			readFileContents: tests.ExampleWedge100MemInfo,
			readFileErr:      nil,
			want: &MemInfo{
				uint64(246900 * 1024),
				uint64(88928 * 1024),
			},
			wantErr: nil,
		},
		{
			name: "unit conversion fail",
			readFileContents: `MemFree: 123 kB
MemTotal: 24`,
			readFileErr: nil,
			want:        nil,
			wantErr:     errors.Errorf("Unable to get MemTotal in /proc/meminfo"),
		},
		{
			name:             "Incomplete meminfo",
			readFileContents: `MemFree: 123 kB`,
			readFileErr:      nil,
			want:             nil,
			wantErr:          errors.Errorf("Unable to get MemTotal in /proc/meminfo"),
		},
		{
			name:             "ReadFile error",
			readFileContents: "",
			readFileErr:      errors.Errorf("ReadFile error"),
			want:             nil,
			wantErr:          errors.Errorf("Unable to open /proc/meminfo: ReadFile error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			ReadFile = func(filename string) ([]byte, error) {
				if filename != "/proc/meminfo" {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", "/proc/meminfo", filename)
				}
				return []byte(tc.readFileContents), tc.readFileErr
			}
			got, err := GetMemInfo()

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
