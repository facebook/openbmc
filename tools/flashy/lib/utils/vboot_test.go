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
	"math"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// test decodeVbs error
func TestDecodeVbs(t *testing.T) {
	cases := []struct {
		name    string
		data    []byte
		wantErr error
	}{
		{
			name:    "not enough bytes",
			data:    []byte{0x00, 0x01},
			wantErr: errors.Errorf("Unable to decode vbs data into struct: unexpected EOF"),
		},
		{
			name:    "empty bytes",
			data:    []byte{},
			wantErr: errors.Errorf("Unable to decode vbs data into struct: EOF"),
		},
		{
			name: "invalid data",
			data: SafeAppendBytes(
				tests.ExampleVbsData[0:AST_SRAM_VBS_SIZE-4],
				[]byte{0x49, 0x59, 0x69, 0x79},
			),
			wantErr: errors.Errorf("CRC16 of vboot data (30049) does not match reference (30425)"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			_, err := decodeVbs(tc.data)
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestVbootPartitionExists(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name            string
		procMtdContents string
		want            bool
	}{
		{
			name:            "rom exists (tiogapass1)",
			procMtdContents: tests.ExampleTiogapass1ProcMtdFile,
			want:            true,
		},
		{
			name:            "rom does not exist (wedge100)",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			want:            false,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != ProcMtdFilePath {
					t.Errorf("filename: want '%v' got '%v'", ProcMtdFilePath, filename)
				}
				return []byte(tc.procMtdContents), nil
			}
			got := vbootPartitionExists()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

// also tests encode and decode
func TestGetVbs(t *testing.T) {
	mmapFileRangeOrig := fileutils.MmapFileRange
	getPageOffsettedOffsetOrig := fileutils.GetPageOffsettedOffset
	vbootPartitionExistsOrig := vbootPartitionExists
	munmapOrig := fileutils.Munmap
	getMachineOrig := getMachine
	defer func() {
		fileutils.MmapFileRange = mmapFileRangeOrig
		fileutils.GetPageOffsettedOffset = getPageOffsettedOffsetOrig
		vbootPartitionExists = vbootPartitionExistsOrig
		fileutils.Munmap = munmapOrig
		getMachine = getMachineOrig
	}()
	vbootPartitionExists = func() bool {
		return true
	}
	getMachine = func() (string, error) {
		return "armv6l", nil
	}
	wantVbs := Vbs{
		671630160,
		88,
		70980,
		671614284,
		0,
		0,
		1,
		1,
		0,
		0,
		0,
		0,
		0,
		30425,
		0,
		1497730626,
		1572486894,
		0,
		1497730626,
		1582846625,
		0,
	}
	fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
		if filename != "/dev/mem" {
			t.Errorf("filename: want '%v' got '%v'", "/dev/mem", filename)
		}
		if offset != AST_SRAM_VBS_BASE {
			t.Errorf("offset: want '%v' got '%v'", AST_SRAM_VBS_BASE, offset)
		}
		return tests.ExampleVbsData, nil
	}
	fileutils.GetPageOffsettedOffset = func(addr uint32) uint32 {
		return 0
	}
	fileutils.Munmap = func(b []byte) (err error) {
		return nil
	}

	got, err := GetVbs()
	if err != nil {
		t.Error(err)
	}
	if !reflect.DeepEqual(wantVbs, got) {
		t.Errorf("want '%v' got '%v'", wantVbs, got)
	}

	data, err := got.encodeVbs()
	if err != nil {
		t.Error(err)
	}
	if !bytes.Equal(data, tests.ExampleVbsData) {
		t.Errorf("encode failed: want '%v' got '%v'", tests.ExampleVbsData, data)
	}

	// mock mmap error
	fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
		if filename != "/dev/mem" {
			t.Errorf("filename: want '%v' got '%v'", "/dev/mem", filename)
		}
		if offset != AST_SRAM_VBS_BASE {
			t.Errorf("offset: want '%v' got '%v'", AST_SRAM_VBS_BASE, offset)
		}
		return tests.ExampleVbsData, errors.Errorf("failed")
	}

	got, err = GetVbs()
	tests.CompareTestErrors(errors.Errorf("Unable to mmap /dev/mem: failed"), err, t)

	// test AST2600 path
	getMachine = func() (string, error) {
		return "armv7l", nil
	}
	fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
		if filename != "/dev/mem" {
			t.Errorf("filename: want '%v' got '%v'", "/dev/mem", filename)
		}
		if offset != AST_SRAM_VBS_BASE_G6 {
			t.Errorf("offset: want '%v' got '%v'", AST_SRAM_VBS_BASE_G6, offset)
		}
		return tests.ExampleVbsData, nil
	}

	got, err = GetVbs()
	if err != nil {
		t.Error(err)
	}

	// vboot partition does not exist
	vbootPartitionExists = func() bool {
		return false
	}

	got, err = GetVbs()
	tests.CompareTestErrors(errors.Errorf("Not a Vboot system: vboot partition (rom) does not exist."),
		err, t)
}

func TestGetVbsOverflowed(t *testing.T) {
	// mock and defer restore MmapFileRange, GetPageOffsettedOffset & vbootPartitionExists
	mmapFileRangeOrig := fileutils.MmapFileRange
	getPageOffsettedOffsetOrig := fileutils.GetPageOffsettedOffset
	vbootPartitionExistsOrig := vbootPartitionExists
	defer func() {
		fileutils.MmapFileRange = mmapFileRangeOrig
		fileutils.GetPageOffsettedOffset = getPageOffsettedOffsetOrig
		vbootPartitionExists = vbootPartitionExistsOrig
	}()
	vbootPartitionExists = func() bool {
		return true
	}
	fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
		return tests.ExampleVbsData, nil
	}
	fileutils.GetPageOffsettedOffset = func(addr uint32) uint32 {
		return math.MaxUint32
	}
	wantErr := errors.Errorf("VBS end offset overflowed: Unsigned integer overflow for (4294967295+56)")

	_, err := GetVbs()
	tests.CompareTestErrors(wantErr, err, t)
}

func TestGetVbootEnforcement(t *testing.T) {
	getVbsOrig := GetVbs
	defer func() {
		GetVbs = getVbsOrig
	}()
	cases := []struct {
		name   string
		vbs    Vbs
		vbsErr error
		want   VbootEnforcementType
	}{
		{
			name:   "GetVbs failed",
			vbs:    Vbs{},
			vbsErr: errors.Errorf("GetVbs failed"),
			want:   VBOOT_NONE,
		},
		{
			name: "not enforced",
			vbs: Vbs{
				Hardware_enforce: 0,
				Software_enforce: 0,
			},
			vbsErr: nil,
			want:   VBOOT_NONE,
		},
		{
			name: "software enforce",
			vbs: Vbs{
				Hardware_enforce: 0,
				Software_enforce: 1,
			},
			vbsErr: nil,
			want:   VBOOT_SOFTWARE_ENFORCE,
		},
		{
			// this does not make sense, but we've seen it in the wild.
			name: "hardware enforce but software enforce 0",
			vbs: Vbs{
				Hardware_enforce: 1,
				Software_enforce: 0,
			},
			vbsErr: nil,
			want:   VBOOT_HARDWARE_ENFORCE,
		},
		{
			name: "hardware enforce",
			vbs: Vbs{
				Hardware_enforce: 1,
				Software_enforce: 1,
			},
			vbsErr: nil,
			want:   VBOOT_HARDWARE_ENFORCE,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			GetVbs = func() (Vbs, error) {
				return tc.vbs, tc.vbsErr
			}

			got := GetVbootEnforcement()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
