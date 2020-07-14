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
package devices

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestGetMTD(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := utils.ReadFile
	defer func() {
		utils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name            string
		specifier       string
		procMtdContents string
		readFileErr     error
		want            *FlashDevice
		wantErr         error
	}{
		{
			name:            "wedge100 example: find mtd:flash0",
			specifier:       "flash0",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			readFileErr:     nil,
			want: &FlashDevice{
				"mtd",
				"flash0",
				"/dev/mtd5",
				uint64(33554432),
				uint64(0),
			},
			wantErr: nil,
		},
		{
			name:            "ReadFile error",
			specifier:       "flash0",
			procMtdContents: "",
			readFileErr:     errors.Errorf("ReadFile error"),
			want:            nil,
			wantErr:         errors.Errorf("Unable to read from /proc/mtd: ReadFile error"),
		},
		{
			name:            "MTD not found in /proc/mtd",
			specifier:       "flash1",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			readFileErr:     nil,
			want:            nil,
			wantErr:         errors.Errorf("Error finding MTD entry in /proc/mtd for flash device 'mtd:flash1'"),
		},
		{
			name:            "size parse error",
			specifier:       "flash1",
			procMtdContents: `mtd0: 10000000000000000 00100000 "flash1"`, // larger than 64-bits
			readFileErr:     nil,
			want:            nil,
			wantErr: errors.Errorf("Found MTD entry for flash device 'mtd:flash1' " +
				"but got error 'strconv.ParseUint: parsing \"10000000000000000\": value out of range'"),
		},
		{
			name:            "corrupt /proc/mtd file",
			specifier:       "flash1",
			procMtdContents: `mtd0: xxxxxxxx xxxxxxxx "flash1"`,
			readFileErr:     nil,
			want:            nil,
			wantErr:         errors.Errorf("Error finding MTD entry in /proc/mtd for flash device 'mtd:flash1'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.ReadFile = func(filename string) ([]byte, error) {
				return []byte(tc.procMtdContents), tc.readFileErr
			}
			got, err := getMTD(tc.specifier)
			tests.CompareTestErrors(tc.wantErr, err, t)

			if got == nil {
				if tc.want != nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				}
			} else {
				if tc.want == nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				} else if !reflect.DeepEqual(*tc.want, *got) {
					t.Errorf("want '%v', got '%v'", tc.want, got)
				}
			}
		})
	}
}
