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

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
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

func TestRunCommand(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	defer func() {
		log.SetOutput(os.Stderr)
	}()

	cases := []struct {
		name             string
		cmdArr           []string
		timeoutInSeconds int
		wantExitCode     int
		wantErr          error
		wantStdout       string
		wantStderr       string
		logContainsSeq   []string // logs must contain all strings
	}{
		{
			name:             "Run `bash -c \"echo openbmc\"`",
			cmdArr:           []string{"bash", "-c", "echo openbmc"},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "openbmc\n",
			wantStderr:       "",
			logContainsSeq: []string{
				"Running command 'bash -c echo openbmc' with 30s timeout",
				"stdout: openbmc\n",
				"Command 'bash -c echo openbmc' exited with code 0 after",
			},
		},
		{
			name:             "Run `bash -c \"echo openbmc 1>&2\"` (to stderr)",
			cmdArr:           []string{"bash", "-c", "echo openbmc 1>&2"},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "",
			wantStderr:       "openbmc\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo openbmc 1>&2' with 30s timeout",
				"stderr: openbmc\n",
				"Command 'bash -c echo openbmc 1>&2' exited with code 0 after",
			},
		},
		{
			name:             "Run `bash -c \"ech0 openbmc\"` (rubbish command)",
			cmdArr:           []string{"bash", "-c", "ech0 openbmc"},
			timeoutInSeconds: 30,
			wantExitCode:     127,
			wantErr:          errors.Errorf("exit status 127"),
			wantStdout:       "",
			wantStderr:       "bash: ech0: command not found\n",
			logContainsSeq: []string{
				"Running command 'bash -c ech0 openbmc' with 30s timeout",
				"stderr: bash: ech0: command not found",
				"Command 'bash -c ech0 openbmc' exited with code 127 after",
			},
		},
		{
			// NOTE:- this is different from bash -c "ech0 openbmc", as Golang's
			// exec.Command takes the first argument as the name of an executable to execute
			// this will have a different error, and expects the default failed exit code (1)
			name:             "Run 'ech0 openbmc'` (rubbish command, not found in PATH)",
			cmdArr:           []string{"ech0", "openbmc"},
			timeoutInSeconds: 30,
			wantExitCode:     1,
			wantErr:          errors.Errorf("exec: \"ech0\": executable file not found in $PATH"),
			wantStdout:       "",
			wantStderr:       "",
			logContainsSeq: []string{
				"Command 'ech0 openbmc' failed to start: exec: \"ech0\": executable file not found in $PATH",
			},
		},
		{
			name:             "Command timed out",
			cmdArr:           []string{"sleep", "42"},
			timeoutInSeconds: 1,
			wantExitCode:     -1,
			wantErr:          errors.Errorf("context deadline exceeded"),
			wantStdout:       "",
			wantStderr:       "",
			logContainsSeq: []string{
				"Running command 'sleep 42' with 1s timeout",
				"Command 'sleep 42' timed out after 1s",
			},
		},
		{
			name: "Check stream sequence: stderr -> stdout -> stderr",
			// the sleeps are necessary as the scanners are concurrrent
			cmdArr:           []string{"bash", "-c", "echo seq1 >&2; sleep 0.1; echo seq2; sleep 0.1; echo seq3 >&2"},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "seq2\n",
			wantStderr:       "seq1\nseq3\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo seq1 >&2; sleep 0.1; echo seq2; sleep 0.1; echo seq3 >&2' with 30s timeout",
				"stderr: seq1\n",
				"stdout: seq2\n",
				"stderr: seq3\n",
			},
		},
		{
			name: "Check stream sequence: stdout -> stderr -> stdout",
			// the sleeps are necessary as the scanners are concurrrent
			cmdArr:           []string{"bash", "-c", "echo seq1; sleep 0.1; echo seq2 >&2; sleep 0.1; echo seq3"},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "seq1\nseq3\n",
			wantStderr:       "seq2\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo seq1; sleep 0.1; echo seq2 >&2; sleep 0.1; echo seq3' with 30s timeout",
				"stdout: seq1\n",
				"stderr: seq2\n",
				"stdout: seq3\n",
			},
		},
		{
			name:             "Non utf-8 character",
			cmdArr:           []string{"printf", "'\x87'"},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "'\x87'\n",
			wantStderr:       "",
			logContainsSeq: []string{
				"Running command 'printf '\x87'",
				"stdout: '\x87'",
			},
		},
		{
			name: "Mock progress bar",
			cmdArr: []string{
				"bash",
				"-c",
				"echo -ne 'PROG:#  \r'; echo -ne 'PROG:## \r'; echo -ne 'PROG:###\r'; echo -ne '\n'",
			},
			timeoutInSeconds: 30,
			wantExitCode:     0,
			wantErr:          nil,
			wantStdout:       "PROG:#  \rPROG:## \rPROG:###\n",
			wantStderr:       "",
			logContainsSeq: []string{
				"stdout: PROG:#  \rPROG:## \rPROG:###\n",
			},
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			exitCode, err, stdoutStr, stderrStr := RunCommand(tc.cmdArr, tc.timeoutInSeconds)

			if exitCode != tc.wantExitCode {
				t.Errorf("exitCode: want %v got %v", tc.wantExitCode, exitCode)
			}

			tests.CompareTestErrors(tc.wantErr, err, t)

			if stdoutStr != tc.wantStdout {
				t.Errorf("stdout: want '%v' got '%v'", tc.wantStdout, stdoutStr)
			}
			if stderrStr != tc.wantStderr {
				t.Errorf("stderr: want '%v' got '%v'", tc.wantStderr, stderrStr)
			}

			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}

func TestSystemdAvailable(t *testing.T) {
	// save and defer restore FileExists and ReadFile
	fileExistsOrig := FileExists
	readFileOrig := ReadFile
	defer func() {
		FileExists = fileExistsOrig
		ReadFile = readFileOrig
	}()

	cases := []struct {
		name             string
		fileExists       bool
		readFileContents string
		readFileErr      error
		want             bool
		wantErr          error
	}{
		{
			name:             "file does not exist",
			fileExists:       false,
			readFileContents: "",
			readFileErr:      nil,
			want:             false,
			wantErr:          nil,
		},
		{
			name:             "systemd not available (as in wedge100)",
			fileExists:       true,
			readFileContents: "init ",
			readFileErr:      nil,
			want:             false,
			wantErr:          nil,
		},
		{
			name:             "systemd available",
			fileExists:       true,
			readFileContents: " systemd ",
			readFileErr:      nil,
			want:             true,
			wantErr:          nil,
		},
		{
			name:             "/proc/1/cmdline exists but ReadFile failed",
			fileExists:       true,
			readFileContents: "",
			readFileErr:      errors.Errorf("ReadFile error"),
			want:             false,
			wantErr:          errors.Errorf("/proc/1/cmdline exists but cannot be read: ReadFile error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			FileExists = func(filename string) bool {
				if filename != "/proc/1/cmdline" {
					t.Errorf("filename: want '%v' got '%v'", "/proc/1/cmdline", filename)
				}
				return tc.fileExists
			}
			ReadFile = func(filename string) ([]byte, error) {
				if filename != "/proc/1/cmdline" {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", "/proc/meminfo", filename)
				}
				return []byte(tc.readFileContents), tc.readFileErr
			}
			got, err := SystemdAvailable()

			tests.CompareTestErrors(tc.wantErr, err, t)

			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
