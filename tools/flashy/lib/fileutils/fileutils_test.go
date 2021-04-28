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
	"time"

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
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			osStat = func(filename string) (os.FileInfo, error) {
				return &mockFileInfo{tc.isDir, nil}, tc.osStatErr
			}
			got := FileExists("x")
			if tc.want != got {
				t.Errorf("want %v got %v", tc.want, got)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestDirExists(t *testing.T) {
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
			name:           "path exists and is dir",
			isDir:          true,
			osStatErr:      nil,
			logContainsSeq: []string{},
			want:           true,
		},
		{
			name:           "path exists and is not dir",
			isDir:          false,
			osStatErr:      nil,
			logContainsSeq: []string{},
			want:           false,
		},
		{
			name:           "path surely does not exist",
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
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			osStat = func(filename string) (os.FileInfo, error) {
				return &mockFileInfo{tc.isDir, nil}, tc.osStatErr
			}
			got := DirExists("x")
			if tc.want != got {
				t.Errorf("want %v got %v", tc.want, got)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
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

func TestGlobAll(t *testing.T) {
	cases := []struct {
		name     string
		patterns []string
		// these tests should be system agnostic so even if the
		// path is valid we cannot test for how many files we got
		// hence, we can only test for the error
		want error
	}{
		{
			name:     "Empty pattern",
			patterns: []string{},
			want:     nil,
		},
		{
			name:     "Test multiple valid patterns",
			patterns: []string{"/var/*", "/var/log/messages"},
			want:     nil,
		},
		{
			name:     "Invalid pattern",
			patterns: []string{"["},
			want:     errors.Errorf("Unable to resolve pattern '[': syntax error in pattern"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			_, got := GlobAll(tc.patterns)

			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestWriteFileWIthTimeout(t *testing.T) {
	writeFileOrig := WriteFile
	defer func() {
		WriteFile = writeFileOrig
	}()

	cases := []struct {
		name      string
		writeErr  error
		writeTime time.Duration
		timeout   time.Duration
		want      error
	}{
		{
			name:      "within timeout",
			writeErr:  nil,
			writeTime: 1 * time.Millisecond,
			timeout:   1 * time.Second,
			want:      nil,
		},
		{
			name:      "within timeout but write errored",
			writeErr:  errors.Errorf("write error"),
			writeTime: 1 * time.Millisecond,
			timeout:   1 * time.Second,
			want:      errors.Errorf("write error"),
		},
		{
			name:      "timeout exceeded",
			writeErr:  nil,
			writeTime: 1 * time.Second,
			timeout:   1 * time.Millisecond,
			want:      errors.Errorf("Timed out after '1ms'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			WriteFile = func(
				filename string,
				data []byte,
				perm os.FileMode,
			) error {
				time.Sleep(tc.writeTime)
				return tc.writeErr
			}

			got := WriteFileWithTimeout("x", []byte("abcd"), 0, tc.timeout)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
