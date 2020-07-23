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

package validate

import (
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestGetOpenBMCVersionFromIssueFile(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := utils.ReadFile
	defer func() {
		utils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name             string
		etcIssueContents string
		etcIssueReadErr  error
		want             string
		wantErr          error
	}{
		{
			name: "example fbtp /etc/issue",
			etcIssueContents: `OpenBMC Release fbtp-v2020.09.1
 `,
			etcIssueReadErr: nil,
			want:            "fbtp-v2020.09.1",
			wantErr:         nil,
		},
		{
			name: "example wedge100 /etc/issue",
			etcIssueContents: `OpenBMC Release wedge100-v2020.07.1
 `,
			etcIssueReadErr: nil,
			want:            "wedge100-v2020.07.1",
			wantErr:         nil,
		},
		{
			name:             "read error",
			etcIssueContents: ``,
			etcIssueReadErr:  errors.Errorf("/etc/issue read error"),
			want:             "",
			wantErr:          errors.Errorf("Error reading /etc/issue: /etc/issue read error"),
		},
		{
			name:             "corrupt /etc/issue",
			etcIssueContents: `OpenBMC Release`,
			etcIssueReadErr:  nil,
			want:             "",
			wantErr: errors.Errorf("Unable to get version from /etc/issue: %v",
				"No match for regex '^OpenBMC Release (?P<version>[^\\s]+)' for input 'OpenBMC Release'"),
		},
		{
			name:             "openbmc wrong case ",
			etcIssueContents: `openbmc Release wedge100-v2020.07.1`,
			etcIssueReadErr:  nil,
			want:             "",
			wantErr: errors.Errorf("Unable to get version from /etc/issue: %v",
				"No match for regex '^OpenBMC Release (?P<version>[^\\s]+)' for input 'openbmc Release wedge100-v2020.07.1'"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/etc/issue" {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", "/etc/issue", filename)
				}
				return []byte(tc.etcIssueContents), tc.etcIssueReadErr
			}
		})
		got, err := getOpenBMCVersionFromIssueFile()

		if tc.want != got {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
		tests.CompareTestErrors(tc.wantErr, err, t)
	}
}

func TestGetOpenBMCVersionFromImageFile(t *testing.T) {
	// mock and defer restore MmapFileRO
	mmapOrig := utils.MmapFileRange
	munmapOrig := utils.Munmap
	defer func() {
		utils.MmapFileRange = mmapOrig
		utils.Munmap = munmapOrig
	}()

	utils.Munmap = func(b []byte) error {
		return nil
	}

	cases := []struct {
		name    string
		fileBuf []byte
		mmapErr error
		want    string
		wantErr error
	}{
		{
			name:    "example tiogapass1 image U-Boot strings",
			fileBuf: []byte(tests.ExampleTiogaPass1ImageUbootStrings),
			mmapErr: nil,
			want:    "fbtp-v2020.09.1",
			wantErr: nil,
		},
		{
			name:    "example wedge100 image U-Boot strings",
			fileBuf: []byte(tests.ExampleWedge100ImageUbootStrings),
			mmapErr: nil,
			want:    "wedge100-v2020.07.1",
			wantErr: nil,
		},
		{
			name:    "no U-Boot entry",
			fileBuf: []byte{},
			mmapErr: nil,
			want:    "",
			wantErr: errors.Errorf("Unable to find OpenBMC version in image file 'x'"),
		},
		{
			name:    "mmap err",
			fileBuf: []byte{},
			mmapErr: errors.Errorf("mmap err"),
			want:    "",
			wantErr: errors.Errorf("Unable to read and mmap image file 'x': mmap err"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
				if filename != "x" {
					t.Errorf("want filename '%v' got '%v'", "x", filename)
				}
				return tc.fileBuf, tc.mmapErr
			}
			got, err := getOpenBMCVersionFromImageFile("x")
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}
