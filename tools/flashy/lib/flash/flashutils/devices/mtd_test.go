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
	"bytes"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// generic tests for MTD
func TestMTD(t *testing.T) {
	m := MemoryTechnologyDevice{
		"flash0",
		"/dev/mtd5",
		uint64(42),
	}

	gotType := m.GetType()
	if "mtd" != gotType {
		t.Errorf("type: want '%v' got '%v'", "mtd", gotType)
	}
	gotSpecifier := m.GetSpecifier()
	if "flash0" != gotSpecifier {
		t.Errorf("specifier: want '%v' got '%v'", "flash0", gotSpecifier)
	}
	gotFilePath := m.GetFilePath()
	if "/dev/mtd5" != gotFilePath {
		t.Errorf("filepath: want '%v' got '%v'", "/dev/mtd5", gotFilePath)
	}
	gotFileSize := m.GetFileSize()
	if uint64(42) != gotFileSize {
		t.Errorf("filesize: want '%v' got '%v'", uint64(42), gotFileSize)
	}
}

func TestGetMTD(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name            string
		specifier       string
		procMtdContents string
		readFileErr     error
		want            FlashDevice
		wantErr         error
	}{
		{
			name:            "wedge100 example: find mtd:flash0",
			specifier:       "flash0",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			readFileErr:     nil,
			want: &MemoryTechnologyDevice{
				"flash0",
				"/dev/mtd5",
				uint64(33554432),
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
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/proc/mtd" {
					t.Errorf("filename: want '%v' got '%v'", "/proc/mtd", filename)
				}
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
				} else if !reflect.DeepEqual(tc.want, got) {
					t.Errorf("want '%v', got '%v'", tc.want, got)
				}
			}
		})
	}
}

func TestMmapRO(t *testing.T) {
	// mock and defer restore MmapFileRange
	mmapFileRangeOrig := fileutils.MmapFileRange
	defer func() {
		fileutils.MmapFileRange = mmapFileRangeOrig
	}()
	cases := []struct {
		name        string
		mtdFilePath string
		mmapRet     []byte
		mmapErr     error
		want        []byte
		wantErr     error
	}{
		{
			name:        "default sucessful operation",
			mtdFilePath: "/dev/mtd5",
			mmapRet:     []byte("abc"),
			mmapErr:     nil,
			want:        []byte("abc"),
			wantErr:     nil,
		},
		{
			name:        "mtd file path invalid causing mmap file path error",
			mtdFilePath: "/dev/mtdx",
			mmapRet:     []byte{},
			mmapErr:     nil,
			want:        []byte{},
			wantErr: errors.Errorf("Unable to get block file path for '/dev/mtdx': " +
				"No match for regex '^(?P<devmtdpath>/dev/mtd)(?P<mtdnum>[0-9]+)$' for " +
				"input '/dev/mtdx'"),
		},
		{
			name:        "mmap error",
			mtdFilePath: "/dev/mtd5",
			mmapRet:     []byte{},
			mmapErr:     errors.Errorf("mmap error"),
			want:        []byte{},
			wantErr:     errors.Errorf("mmap error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			testMtd := MemoryTechnologyDevice{
				"flash0",
				tc.mtdFilePath,
				uint64(1234),
			}
			fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
				return tc.mmapRet, tc.mmapErr
			}
			got, err := testMtd.MmapRO()

			tests.CompareTestErrors(tc.wantErr, err, t)
			if bytes.Compare(tc.want, got) != 0 {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestGetMmapFilePath(t *testing.T) {
	cases := []struct {
		name        string
		mtdFilePath string
		want        string
		wantErr     error
	}{
		{
			name:        "/dev/mtd5 test",
			mtdFilePath: "/dev/mtd5",
			want:        "/dev/mtdblock5",
			wantErr:     nil,
		},
		{
			name:        "/dev/mtd12 test",
			mtdFilePath: "/dev/mtd12",
			want:        "/dev/mtdblock12",
			wantErr:     nil,
		},
		{
			name:        "invalid mtd file path",
			mtdFilePath: "/dev/mtddddd",
			want:        "",
			wantErr: errors.Errorf("Unable to get block file path for '/dev/mtddddd': " +
				"No match for regex '^(?P<devmtdpath>/dev/mtd)(?P<mtdnum>[0-9]+)$' for input '/dev/mtddddd'"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			mtd := MemoryTechnologyDevice{
				"foobar",
				tc.mtdFilePath,
				uint64(123),
			}
			got, err := mtd.getMmapFilePath()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestValidate(t *testing.T) {
	// mock and defer restore Validate, MmapFileRange & Munmap
	validateOrig := validate.Validate
	mmapFileRangeOrig := fileutils.MmapFileRange
	munmapOrig := fileutils.Munmap
	defer func() {
		validate.Validate = validateOrig
		fileutils.MmapFileRange = mmapFileRangeOrig
		fileutils.Munmap = munmapOrig
	}()

	cases := []struct {
		name        string
		mmapErr     error
		validateErr error
		want        error
	}{
		{
			name:        "succeeded",
			mmapErr:     nil,
			validateErr: nil,
			want:        nil,
		},
		{
			name:        "mmap failed",
			mmapErr:     errors.Errorf("mmap failed"),
			validateErr: nil,
			want:        errors.Errorf("Can't mmap flash device: mmap failed"),
		},
		{
			name:        "validation failed",
			mmapErr:     nil,
			validateErr: errors.Errorf("Validation failed"),
			want:        errors.Errorf("Validation failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			m := &MemoryTechnologyDevice{
				"flash0",
				"/dev/mtd5",
				uint64(33554432),
			}
			exampleData := []byte("abcd")
			fileutils.MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
				if filename != "/dev/mtdblock5" {
					t.Errorf("filename: want '%v' got '%v'", "/dev/mtdblock5", filename)
				}
				if length != 33554432 {
					t.Errorf("length: want '%v' got '%v'", 33554432, length)
				}
				return exampleData, tc.mmapErr
			}
			wantMunmapCalled := tc.mmapErr == nil
			munmapCalled := false
			fileutils.Munmap = func(data []byte) error {
				munmapCalled = true
				return nil
			}
			validate.Validate = func(data []byte) error {
				if !reflect.DeepEqual(exampleData, data) {
					t.Errorf("data: want '%v' got '%v'", exampleData, data)
				}
				return tc.validateErr
			}
			got := m.Validate()
			tests.CompareTestErrors(tc.want, got, t)
			if wantMunmapCalled != munmapCalled {
				t.Errorf("munmapCalled: want '%v' got '%v'",
					wantMunmapCalled, munmapCalled)
			}
		})
	}
}

func TestGetWritableMountedMTDs(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name               string
		procMountsContents string
		readFileErr        error
		want               []WritableMountedMTD
		wantErr            error
	}{
		{
			name:               "Example wedge100 /proc/mounts",
			procMountsContents: tests.ExampleWedge100ProcMountsFile,
			readFileErr:        nil,
			want: []WritableMountedMTD{
				WritableMountedMTD{
					"/dev/mtdblock4",
					"/mnt/data",
				},
			},
			wantErr: nil,
		},
		{
			name:               "No writable mounted MTD found",
			procMountsContents: "",
			readFileErr:        nil,
			want:               []WritableMountedMTD{},
			wantErr:            nil,
		},
		{
			name: "Multiple writable mounted MTDs",
			procMountsContents: `/dev/mtdblock4 /mnt/data jffs2 rw,relatime 0 0
/dev/mtd1 /mnt/data2 jffs2 relatime,rw 0 0
/dev/mtd2 /mnt/data3 jffs2 relatime 0 0`,
			readFileErr: nil,
			want: []WritableMountedMTD{
				WritableMountedMTD{
					"/dev/mtdblock4",
					"/mnt/data",
				},
				WritableMountedMTD{
					"/dev/mtd1",
					"/mnt/data2",
				},
			},
			wantErr: nil,
		},
		{
			name:               "ReadFile Error",
			procMountsContents: "",
			readFileErr:        errors.Errorf("ReadFile error"),
			want:               []WritableMountedMTD{},
			wantErr: errors.Errorf("Unable to get writable mounted MTDs: " +
				"Cannot read /proc/mounts: ReadFile error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				return []byte(tc.procMountsContents), tc.readFileErr
			}

			got, err := GetWritableMountedMTDs()

			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v', got '%v'", tc.want, got)
			}
		})
	}
}
