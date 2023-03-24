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
	"strings"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestCompatibleVersionMapping(t *testing.T) {
	// the values of compatibleVersionMapping cannot have a dash
	for key, val := range compatibleVersionMapping {
		if strings.ContainsAny(val, "-") {
			t.Errorf("Invalid mapping (%v, %v), value %v cannot contain a dash",
				key, val, val)
		}
	}
}

func TestCheckImageBuildNameCompatibility(t *testing.T) {
	getOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile
	getOpenBMCVersionFromImageFileOrig := getOpenBMCVersionFromImageFile
	compatibleVersionMappingOrig := compatibleVersionMapping
	defer func() {
		utils.GetOpenBMCVersionFromIssueFile = getOpenBMCVersionFromIssueFileOrig
		getOpenBMCVersionFromImageFile = getOpenBMCVersionFromImageFileOrig
		compatibleVersionMapping = compatibleVersionMappingOrig
	}()

	cases := []struct {
		name         string
		etcIssueVer  string
		imageFileVer string
		want         error
	}{
		{
			name:         "match passing",
			etcIssueVer:  "fbtp-v2020.09.1",
			imageFileVer: "fbtp-v2019.01.3",
			want:         nil,
		},
		{
			name:         "does not match",
			etcIssueVer:  "fbtp-v2020.09.1",
			imageFileVer: "yosemite-v1.2",
			want:         errors.Errorf("OpenBMC versions from /etc/issue ('fbtp') and image file ('yosemite') do not match!"),
		},
		{
			name:         "compatible, uses normalized version",
			etcIssueVer:  "fby2-gpv2-v2019.43.1",
			imageFileVer: "fbgp2-v1234",
			want:         nil,
		},
		{
			name:         "etc issue build name get error",
			etcIssueVer:  "!@#$",
			imageFileVer: "yfbgp2-v1234",
			want: errors.Errorf(
				"Unable to get build name from version '!@#$' (normalized: '!@#$'): " +
					"No match for regex '^(?P<buildname>\\w+)' for input '!@#$'",
			),
		},
		{
			name:         "image build name get error",
			etcIssueVer:  "fby2-gpv2-v2019.43.1",
			imageFileVer: "!@#$",
			want: errors.Errorf(
				"Unable to get build name from version '!@#$' (normalized: '!@#$'): " +
					"No match for regex '^(?P<buildname>\\w+)' for input '!@#$'",
			),
		},
		{
			name:         "ancient image",
			etcIssueVer:  "unknown-v52.1",
			imageFileVer: "wedge40-v2019.01.3",
			want:         nil,
		},
		{
			name:         "older image",
			etcIssueVer:  "unknown-v42",
			imageFileVer: "wedge40-v2019.01.3",
			want:         nil,
		},
	}

	compatibleVersionMapping = map[string]string{"fby2-gpv2": "fbgp2"}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleImageFilePath := "/run/upgrade/mock"
			utils.GetOpenBMCVersionFromIssueFile = func() (string, error) {
				return tc.etcIssueVer, nil
			}
			getOpenBMCVersionFromImageFile = func(imageFilePath string) (string, error) {
				if exampleImageFilePath != imageFilePath {
					t.Errorf("imageFilePath: want '%v' got '%v'", exampleImageFilePath, imageFilePath)
				}
				return tc.imageFileVer, nil
			}
			got := CheckImageBuildNameCompatibility(exampleImageFilePath)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

// also tests normalizeVersion
func TestGetNormalizedBuildNameFromVersion(t *testing.T) {
	compatibleVersionMappingOrig := compatibleVersionMapping
	defer func() {
		compatibleVersionMapping = compatibleVersionMappingOrig
	}()

	cases := []struct {
		name    string
		ver     string
		want    string
		wantErr error
	}{
		{
			name:    "wedge100 example",
			ver:     "wedge100-v2020.07.1",
			want:    "wedge100",
			wantErr: nil,
		},
		{
			name:    "tiogapass1 example",
			ver:     "fbtp-v2020.09.1",
			want:    "fbtp",
			wantErr: nil,
		},
		{
			name:    "normalization test (fby2-gpv2)",
			ver:     "fby2-gpv2-v2019.43.1",
			want:    "fbgp2",
			wantErr: nil,
		},
		{
			name:    "ancient example",
			ver:     "unknown-v52.4",
			want:    "unknown",
			wantErr: nil,
		},
		{
			name: "rubbish version, no match",
			ver:  "!@#$",
			want: "",
			wantErr: errors.Errorf("Unable to get build name from version '%v' (normalized: '%v'): %v",
				"!@#$", "!@#$", "No match for regex '^(?P<buildname>\\w+)' for input '!@#$'"),
		},
	}

	compatibleVersionMapping = map[string]string{"fby2-gpv2": "fbgp2"}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := getNormalizedBuildNameFromVersion(tc.ver)

			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestGetOpenBMCVersionFromImageFile(t *testing.T) {
	// mock and defer restore MmapFileRO
	mmapOrig := fileutils.MmapFileRange
	munmapOrig := fileutils.Munmap
	defer func() {
		fileutils.MmapFileRange = mmapOrig
		fileutils.Munmap = munmapOrig
	}()

	fileutils.Munmap = func(b []byte) error {
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
			wantErr: errors.Errorf("Unable to find OpenBMC version in image file 'x': " +
				"No match for regex 'U-Boot \\d+\\.\\d+ (?P<version>[^\\s]+)' for input"),
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
			fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
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
