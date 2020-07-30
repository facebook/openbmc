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

package fileutils

import (
	"bytes"
	"log"
	"os"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestPathExists(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore osStat
	osStatOrig := osStat
	defer func() {
		log.SetOutput(os.Stderr)
		osStat = osStatOrig
	}()

	cases := []struct {
		name           string
		osStatErr      error
		logContainsSeq []string
		want           bool
	}{
		{
			name:           "exists",
			osStatErr:      nil,
			logContainsSeq: []string{},
			want:           true,
		},
		{
			name:           "surely does not exists",
			osStatErr:      os.ErrNotExist,
			logContainsSeq: []string{},
			want:           false,
		},
		{
			name:      "ambiguous, log and default to false",
			osStatErr: errors.Errorf("12345"),
			logContainsSeq: []string{
				"Existence check of path 'x' returned error '12345', defaulting to false",
			},
			want: false,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			osStat = func(filename string) (os.FileInfo, error) {
				return nil, tc.osStatErr
			}
			got := PathExists("x")
			if tc.want != got {
				t.Errorf("want %v got %v", tc.want, got)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

type mockFileInfo struct {
	isDir bool
	os.FileInfo
}

func (m *mockFileInfo) IsDir() bool {
	return m.isDir
}

func TestFileExists(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore osStat
	osStatOrig := osStat
	defer func() {
		log.SetOutput(os.Stderr)
		osStat = osStatOrig
	}()
	cases := []struct {
		name           string
		isDir          bool
		osStatErr      error
		logContainsSeq []string
		want           bool
	}{
		{
			name:           "file exists and is not dir",
			isDir:          false,
			osStatErr:      nil,
			logContainsSeq: []string{},
			want:           true,
		},
		{
			name:           "file exists and is dir",
			isDir:          true,
			osStatErr:      nil,
			logContainsSeq: []string{},
			want:           false,
		},
		{
			name:           "file surely does not exist",
			isDir:          false,
			osStatErr:      os.ErrNotExist,
			logContainsSeq: []string{},
			want:           false,
		},
		{
			name:      "ambiguous, log and default to false",
			isDir:     false,
			osStatErr: errors.Errorf("12345"),
			logContainsSeq: []string{
				"Existence check of path 'x' returned error '12345', defaulting to false",
			},
			want: false,
		},
	}

	for _, tc := range cases {
		buf = bytes.Buffer{}
		osStat = func(filename string) (os.FileInfo, error) {
			return &mockFileInfo{tc.isDir, nil}, tc.osStatErr
		}
		got := FileExists("x")
		if tc.want != got {
			t.Errorf("want %v got %v", tc.want, got)
		}
		tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
	}
}

func TestIsELFFile(t *testing.T) {
	// mock and defer restore MmapFileRange
	mmapFileRangeOrig := MmapFileRange
	defer func() {
		MmapFileRange = mmapFileRangeOrig
	}()

	cases := []struct {
		name    string
		mmapRet []byte
		mmapErr error
		want    bool
	}{
		{
			name:    "ELF file",
			mmapRet: []byte{0x7F, 'E', 'L', 'F'},
			mmapErr: nil,
			want:    true,
		},
		{
			name:    "not ELF file",
			mmapRet: []byte{0x7F, 'A', 'B', 'C'},
			mmapErr: nil,
			want:    false,
		},
		{
			name:    "mmap error",
			mmapRet: nil,
			mmapErr: errors.Errorf("mmap error"),
			want:    false,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			MmapFileRange = func(filename string, offset int64, length, prot, flags int) ([]byte, error) {
				return tc.mmapRet, tc.mmapErr
			}
			got := IsELFFile("x")
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
