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

package partition

import (
	"bytes"
	"log"
	"os"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// Checks to make sure ImageFormat is properly entered
// (1) Make sure two adjacent partitions don't overlap
// (2) Make sure Offset and Size are multiples of 1024 (prevent accidental kB entry)
//     As of now we don't have any sizes/offsets requiring a higher precision.
// (3) Make sure FIT partitions specify FitImageNodes (fw-util assumes 1 if not specified)
// (4) Make sure final Size + Offset <= 32768 * 1024 (32MB)
func TestImageFormats(t *testing.T) {
	var validateFormat = func(format ImageFormat) {
		lastEndOffset := uint32(0)
		for _, config := range format.PartitionConfigs {
			if config.Offset%1024 != 0 {
				t.Errorf("'%v' format '%v' partition offset is not a multiple of 1024, "+
					"did you forget to enter it in bytes?",
					format.Name, config.Name)
			}
			if config.Size%1024 != 0 {
				t.Errorf("'%v' format '%v' partition size is not a multiple of 1024, "+
					"did you forget to enter it in bytes?",
					format.Name, config.Name)
			}
			if config.Offset < lastEndOffset {
				t.Errorf("'%v' format '%v' partition overlapped with previous partition region.",
					format.Name, config.Name)
			}
			if config.Type == FIT {
				if config.FitImageNodes == 0 {
					t.Errorf("'%v' format '%v' partition is FIT but FitImageNodes is not specified",
						format.Name, config.Name)
				}
			}

			lastEndOffset = config.Size + config.Offset
		}
		if lastEndOffset > 32768*1024 {
			t.Errorf("Image format %v' more than 32MB!", format)
		}
	}

	for _, format := range defaultImageFormats {
		validateFormat(format)
	}
	for _, format := range grandCanyonImageFormat {
		validateFormat(format)
	}
}

func TestGetImageFormats(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)

	getOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile
	defer func() {
		log.SetOutput(os.Stderr)
		utils.GetOpenBMCVersionFromIssueFile = getOpenBMCVersionFromIssueFileOrig
	}()

	cases := []struct {
		name           string
		obmcVer        string
		obmcVerErr     error
		logContainsSeq []string
		want           []ImageFormat
	}{
		{
			name:    "OK",
			obmcVer: "wedge100-v2020.07.1",
			want:    defaultImageFormats,
		},
		{
			name:    "Grandcanyon",
			obmcVer: "grandcanyon-v2020.07.1",
			want:    grandCanyonImageFormat,
		},
		{
			name:       "Failed to get OpenBMC version",
			obmcVerErr: errors.Errorf("Zucked"),
			logContainsSeq: []string{"Unable to get OpenBMC version from /etc/issue",
				"Defaulting to default image formats for validation"},
			want: defaultImageFormats,
		},
		{
			name:    "Failed to get build name",
			obmcVer: "!!!!!",
			logContainsSeq: []string{"Unable to get normalized build name from issue file version",
				"Defaulting to default image formats for validation"},
			want: defaultImageFormats,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetOpenBMCVersionFromIssueFile = func() (string, error) {
				return tc.obmcVer, tc.obmcVerErr
			}
			buf = bytes.Buffer{}

			got := GetImageFormats()
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}

			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
