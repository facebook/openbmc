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
	"fmt"
	"log"
	"os"
	"reflect"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestGetMemInfo(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() { fileutils.ReadFile = readFileOrig }()

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
			fileutils.ReadFile = func(filename string) ([]byte, error) {
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
	// set locale (LC_ALL=C) to prevent tests from failing on differing locales
	const lcAllKey = "LC_ALL"
	origLCALL, setLCALL := os.LookupEnv(lcAllKey)
	err := os.Setenv(lcAllKey, "C")
	if err != nil {
		panic(err)
	}
	defer func() {
		if setLCALL {
			err = os.Setenv(lcAllKey, origLCALL)
			if err != nil {
				panic(err)
			}
		} else {
			err = os.Unsetenv(lcAllKey)
			if err != nil {
				panic(err)
			}
		}
		log.SetOutput(os.Stderr)
	}()

	cases := []struct {
		name           string
		cmdArr         []string
		timeout        time.Duration
		wantExitCode   int
		wantErr        error
		wantStdout     string
		wantStderr     string
		logContainsSeq []string // logs must contain all strings
	}{
		{
			name:         "Run `bash -c \"echo openbmc\"`",
			cmdArr:       []string{"bash", "-c", "echo openbmc"},
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "openbmc\n",
			wantStderr:   "",
			logContainsSeq: []string{
				"Running command 'bash -c echo openbmc' with 30s timeout",
				"stdout: openbmc\n",
				"Command 'bash -c echo openbmc' exited with code 0 after",
			},
		},
		{
			name:         "Run `bash -c \"echo openbmc 1>&2\"` (to stderr)",
			cmdArr:       []string{"bash", "-c", "echo openbmc 1>&2"},
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "",
			wantStderr:   "openbmc\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo openbmc 1>&2' with 30s timeout",
				"stderr: openbmc\n",
				"Command 'bash -c echo openbmc 1>&2' exited with code 0 after",
			},
		},
		{
			name:         "Run `bash -c \"ech0 openbmc\"` (rubbish command)",
			cmdArr:       []string{"bash", "-c", "ech0 openbmc"},
			timeout:      30 * time.Second,
			wantExitCode: 127,
			wantErr:      errors.Errorf("exit status 127"),
			wantStdout:   "",
			wantStderr:   "bash: ech0: command not found\n",
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
			name:         "Run 'ech0 openbmc'` (rubbish command, not found in PATH)",
			cmdArr:       []string{"ech0", "openbmc"},
			timeout:      30 * time.Second,
			wantExitCode: 1,
			wantErr:      errors.Errorf("exec: \"ech0\": executable file not found in $PATH"),
			wantStdout:   "",
			wantStderr:   "",
			logContainsSeq: []string{
				"Command 'ech0 openbmc' failed to start: exec: \"ech0\": executable file not found in $PATH",
			},
		},
		{
			name:         "Command timed out",
			cmdArr:       []string{"sleep", "1"},
			timeout:      10 * time.Millisecond,
			wantExitCode: -1,
			wantErr:      errors.Errorf("context deadline exceeded"),
			wantStdout:   "",
			wantStderr:   "",
			logContainsSeq: []string{
				"Running command 'sleep 1' with 10ms timeout",
				"Command 'sleep 1' timed out after 10ms",
			},
		},
		{
			name: "Check stream sequence: stderr -> stdout -> stderr",
			// the sleeps are necessary as the scanners are concurrent
			cmdArr:       []string{"bash", "-c", "echo seq1 >&2; sleep 0.1; echo seq2; sleep 0.1; echo seq3 >&2"},
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "seq2\n",
			wantStderr:   "seq1\nseq3\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo seq1 >&2; sleep 0.1; echo seq2; sleep 0.1; echo seq3 >&2' with 30s timeout",
				"stderr: seq1\n",
				"stdout: seq2\n",
				"stderr: seq3\n",
			},
		},
		{
			name: "Check stream sequence: stdout -> stderr -> stdout",
			// the sleeps are necessary as the scanners are concurrent
			cmdArr:       []string{"bash", "-c", "echo seq1; sleep 0.1; echo seq2 >&2; sleep 0.1; echo seq3"},
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "seq1\nseq3\n",
			wantStderr:   "seq2\n",
			logContainsSeq: []string{
				"Running command 'bash -c echo seq1; sleep 0.1; echo seq2 >&2; sleep 0.1; echo seq3' with 30s timeout",
				"stdout: seq1\n",
				"stderr: seq2\n",
				"stdout: seq3\n",
			},
		},
		{
			name:         "Non utf-8 character",
			cmdArr:       []string{"printf", "'\x87'"},
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "'\x87'\n",
			wantStderr:   "",
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
			timeout:      30 * time.Second,
			wantExitCode: 0,
			wantErr:      nil,
			wantStdout:   "PROG:#  \nPROG:## \nPROG:###\n",
			wantStderr:   "",
			logContainsSeq: []string{
				"stdout: PROG:#",
				"stdout: PROG:##",
				"stdout: PROG:###",
			},
		},
		{
			name:         "Invalid timeout (negative)",
			cmdArr:       []string{"sleep", "42"},
			timeout:      -1 * time.Second,
			wantExitCode: 1,
			wantErr:      errors.Errorf("context deadline exceeded"),
			wantStdout:   "",
			wantStderr:   "",
			logContainsSeq: []string{
				"Running command 'sleep 42' with -1s timeout",
				"Command 'sleep 42' failed to start: context deadline exceeded",
			},
		},
		{
			name:           "Empty cmd",
			cmdArr:         []string{},
			timeout:        30 * time.Second,
			wantExitCode:   1,
			wantErr:        errors.Errorf("Cannot run empty command"),
			wantStdout:     "",
			wantStderr:     "",
			logContainsSeq: []string{},
		},
		{
			name:           "Single item cmd (echo)",
			cmdArr:         []string{"echo"},
			timeout:        30 * time.Second,
			wantExitCode:   0,
			wantErr:        nil,
			wantStdout:     "\n",
			wantStderr:     "",
			logContainsSeq: []string{},
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			exitCode, err, stdoutStr, stderrStr := RunCommand(tc.cmdArr, tc.timeout)

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

func TestRunCommandWithRetries(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// mock and defer restore Sleep and RunCommand
	sleepFuncOrig := Sleep
	runCommandOrig := RunCommand
	defer func() {
		log.SetOutput(os.Stderr)
		Sleep = sleepFuncOrig
		RunCommand = runCommandOrig
	}()

	cases := []struct {
		name           string
		maxAttempts    int
		interval       time.Duration
		runCommandErrs []error
		wantErr        error
		wantSleepTimes []time.Duration
		logContainsSeq []string
	}{
		{
			name:           "Succeed on first try",
			maxAttempts:    3,
			interval:       1 * time.Second,
			runCommandErrs: []error{nil},
			wantErr:        nil,
			wantSleepTimes: []time.Duration{},
			logContainsSeq: []string{
				fmt.Sprintf("Attempt %v of %v: Running command '%v' with timeout %vs and retry interval %vs",
					1, 3, "echo 1", 10, 1),
				"Attempt 1 of 3 succeeded",
			},
		},
		{
			name:           "Succeed on second try",
			maxAttempts:    3,
			interval:       1 * time.Second,
			runCommandErrs: []error{errors.Errorf("err"), nil},
			wantErr:        nil,
			wantSleepTimes: []time.Duration{1 * time.Second},
			logContainsSeq: []string{
				fmt.Sprintf("Attempt %v of %v: Running command '%v' with timeout %vs and retry interval %vs",
					1, 3, "echo 1", 10, 1),
				"Attempt 1 of 3 failed",
				"Sleeping for 1s before retrying",
				"Attempt 2 of 3:",
				"Attempt 2 of 3 succeeded",
			},
		},
		{
			name:           "Succeed on second try, different timeout",
			maxAttempts:    3,
			interval:       42 * time.Second,
			runCommandErrs: []error{errors.Errorf("err"), nil},
			wantErr:        nil,
			wantSleepTimes: []time.Duration{42 * time.Second},
			logContainsSeq: []string{
				fmt.Sprintf("Attempt %v of %v: Running command '%v' with timeout %vs and retry interval %vs",
					1, 3, "echo 1", 10, 42),
				"Attempt 1 of 3 failed",
				"Sleeping for 42s before retrying",
				"Attempt 2 of 3:",
				"Attempt 2 of 3 succeeded",
			},
		},
		{
			name:           "Fail on all retries",
			maxAttempts:    3,
			interval:       1 * time.Second,
			runCommandErrs: []error{errors.Errorf("err"), errors.Errorf("err"), errors.Errorf("err")},
			wantErr:        errors.Errorf("err"),
			wantSleepTimes: []time.Duration{1 * time.Second, 1 * time.Second},
			logContainsSeq: []string{
				fmt.Sprintf("Attempt %v of %v: Running command '%v' with timeout %vs and retry interval %vs",
					1, 3, "echo 1", 10, 1),
				"Attempt 1 of 3 failed",
				"Attempt 2 of 3 failed",
				"Attempt 3 of 3 failed",
				"Max attempts (3) reached. Returning with error.",
			},
		},
		{
			name:           "Invalid maxAttempts (<1)",
			maxAttempts:    0,
			interval:       1 * time.Second,
			runCommandErrs: []error{},
			wantErr:        errors.Errorf("Command failed to run: maxAttempts must be > 0 (got 0)"),
			wantSleepTimes: []time.Duration{},
			logContainsSeq: []string{},
		},
	}

	cmdArr := []string{"echo", "1"}
	timeout := 10 * time.Second

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			attempt := 0
			buf = bytes.Buffer{}
			gotSleepTimes := []time.Duration{}

			RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				// only error is used
				cmdErr := tc.runCommandErrs[attempt]
				attempt++
				return 0, cmdErr, "", ""
			}

			Sleep = func(d time.Duration) {
				gotSleepTimes = append(gotSleepTimes, d)
			}

			_, got, _, _ := RunCommandWithRetries(cmdArr, timeout, tc.maxAttempts, tc.interval)

			tests.CompareTestErrors(tc.wantErr, got, t)
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
			if !reflect.DeepEqual(tc.wantSleepTimes, gotSleepTimes) {
				t.Errorf("sleeptimes: want '%v' got '%v'", tc.wantSleepTimes, gotSleepTimes)
			}
		})
	}
}

func TestSystemdAvailable(t *testing.T) {
	// save and defer restore FileExists and ReadFile
	fileExistsOrig := fileutils.FileExists
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.FileExists = fileExistsOrig
		fileutils.ReadFile = readFileOrig
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
			fileutils.FileExists = func(filename string) bool {
				if filename != "/proc/1/cmdline" {
					t.Errorf("filename: want '%v' got '%v'", "/proc/1/cmdline", filename)
				}
				return tc.fileExists
			}
			fileutils.ReadFile = func(filename string) ([]byte, error) {
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

func TestGetOpenBMCVersionFromIssueFile(t *testing.T) {
	// save and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
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
			name: "ancient /etc/issue",
			etcIssueContents: `Open BMC Release v52.1
  `,
			etcIssueReadErr: nil,
			want:            "unknown-v52.1",
			wantErr:         nil,
		},
		{
			name: "older /etc/issue",
			etcIssueContents: `OpenBMC Release v42
  `,
			etcIssueReadErr: nil,
			want:            "unknown-v42",
			wantErr:         nil,
		},
		{
			name:             "ancient galaxy100 /etc/issue",
			etcIssueContents: `OpenBMC Release 
`,
			etcIssueReadErr:  nil,
			want:             "unknown-v1",
			wantErr:          nil,
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
				"No match for regex '^Open ?BMC Release (?P<version>[^\\s]+)' for input 'OpenBMC Release'"),
		},
		{
			name:             "openbmc wrong case ",
			etcIssueContents: `openbmc Release wedge100-v2020.07.1`,
			etcIssueReadErr:  nil,
			want:             "",
			wantErr: errors.Errorf("Unable to get version from /etc/issue: %v",
				"No match for regex '^Open ?BMC Release (?P<version>[^\\s]+)' for input 'openbmc Release wedge100-v2020.07.1'"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/etc/issue" {
					return []byte{}, errors.Errorf("filename: want '%v' got '%v'", "/etc/issue", filename)
				}
				return []byte(tc.etcIssueContents), tc.etcIssueReadErr
			}
		})
		got, err := GetOpenBMCVersionFromIssueFile()

		if tc.want != got {
			t.Errorf("want '%v' got '%v'", tc.want, got)
		}
		tests.CompareTestErrors(tc.wantErr, err, t)
	}
}

func TestIsOpenBMC(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()
	cases := []struct {
		name             string
		readFileContents string
		readFileError    error
		want             bool
		wantErr          error
	}{
		{
			name: "Is OpenBMC",
			readFileContents: `OpenBMC Release wedge100-v2020.07.1
			 `,
			readFileError: nil,
			want:          true,
			wantErr:       nil,
		},
		{
			name:             "Not OpenBMC",
			readFileContents: `foobar`,
			readFileError:    nil,
			want:             false,
			wantErr:          nil,
		},
		{
			name:             "/etc/issue file read error",
			readFileContents: "",
			readFileError:    errors.Errorf("file read error"),
			want:             false,
			wantErr: errors.Errorf("Error reading '/etc/issue': " +
				"file read error"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/etc/issue" {
					t.Errorf("filename: want '%v' got '%v'",
						"/etc/issue", filename)
				}
				return []byte(tc.readFileContents), tc.readFileError
			}

			got, err := IsOpenBMC()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}

func TestIsLFOpenBMC(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()
	cases := []struct {
		name             string
		readFileContents string
		readFileError    error
		want             bool
	}{
		{
			name: "Is LFOpenBMC",
			readFileContents: `OPENBMC_TARGET_MACHINE=foo`,
			readFileError: nil,
			want:          true,
		},
		{
			name:             "Not OpenBMC",
			readFileContents: `foobar`,
			readFileError:    nil,
			want:             false,
		},
		{
			name:             "/etc/os-release file read error",
			readFileContents: "",
			readFileError:    errors.Errorf("file read error"),
			want:             false,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/etc/os-release" {
					t.Errorf("filename: want '%v' got '%v'",
						"/etc/os-release", filename)
				}
				return []byte(tc.readFileContents), tc.readFileError
			}

			got := IsLFOpenBMC()
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}


func TestCheckOtherFlasherRunning(t *testing.T) {
	// mock and defer restore
	// getOtherCmdlines and checkNoBaseNameExistsInCmdlines
	getOtherProcCmdlinePathsOrig := getOtherProcCmdlinePaths
	checkNoBaseNameExistsInProcCmdlinePathsOrig := checkNoBaseNameExistsInProcCmdlinePaths
	defer func() {
		getOtherProcCmdlinePaths = getOtherProcCmdlinePathsOrig
		checkNoBaseNameExistsInProcCmdlinePaths = checkNoBaseNameExistsInProcCmdlinePathsOrig
	}()

	cases := []struct {
		name                     string
		otherCmdlinesRet         []string
		checkNoBaseNameExistsErr error
		want                     error
	}{
		{
			name: "basic succeeding",
			otherCmdlinesRet: []string{
				"/proc/42/cmdline",
			},
			checkNoBaseNameExistsErr: nil,
			want:                     nil,
		},
		{
			name:                     "baseName found in cmdline",
			otherCmdlinesRet:         nil,
			checkNoBaseNameExistsErr: errors.Errorf("Found another flasher"),
			want:                     errors.Errorf("Another flasher running: Found another flasher"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			getOtherProcCmdlinePaths = func() []string {
				return tc.otherCmdlinesRet
			}
			exampleFlashyStepBaseNames := []string{"foo", "bar"}
			checkNoBaseNameExistsInProcCmdlinePaths = func(baseNames, cmdlines []string) error {
				wantBaseNames := SafeAppendString(otherFlasherBaseNames, exampleFlashyStepBaseNames)
				if !reflect.DeepEqual(wantBaseNames, baseNames) {
					t.Errorf("basenames: want '%v' got '%v'", wantBaseNames, baseNames)
				}
				if !reflect.DeepEqual(tc.otherCmdlinesRet, cmdlines) {
					t.Errorf("cmdlines: want '%v' got '%v'",
						tc.otherCmdlinesRet, cmdlines)
				}
				return tc.checkNoBaseNameExistsErr
			}

			got := CheckOtherFlasherRunning(exampleFlashyStepBaseNames)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestGetOtherProcCmdlinePaths(t *testing.T) {
	// mock and defer restore Glob and ownCmdlines
	globOrig := fileutils.Glob
	ownCmdlinesOrig := ownCmdlines
	defer func() {
		fileutils.Glob = globOrig
		ownCmdlines = ownCmdlinesOrig
	}()

	ownCmdlines = []string{
		"/proc/self/cmdline",
		"/proc/thread-self/cmdline",
	}
	globRet := []string{
		"foo",
		"bar",
		"/proc/self/cmdline",
		"/proc/thread-self/cmdline",
	}
	want := []string{"foo", "bar"}
	fileutils.Glob = func(pattern string) (matches []string, err error) {
		return globRet, nil
	}
	got := getOtherProcCmdlinePaths()
	if !reflect.DeepEqual(want, got) {
		t.Errorf("want '%v' got '%v'", want, got)
	}
}

func TestCheckNoBaseNameExistsInProcCmdlinePaths(t *testing.T) {
	// mock and defer restore ReadFile
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	type ReadFileRetType struct {
		Buf []byte
		Err error
	}

	baseNames := []string{
		"improve_system.py",
		"fw-util",
		"flashy",
		"00_dummy_step",
	}

	cases := []struct {
		name string
		// map from file name to the return
		// the keys of this map are cmdlines
		readFileRet map[string]interface{}
		want        error
	}{
		{
			name: "no basename exists in cmdlines",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline":  ReadFileRetType{[]byte("python\x00test.py"), nil},
				"/proc/420/cmdline": ReadFileRetType{[]byte("ls\x00-la"), nil},
			},
			want: nil,
		},
		{
			name: "ignore readfile errors",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline":  ReadFileRetType{[]byte{}, errors.Errorf("!")},
				"/proc/420/cmdline": ReadFileRetType{[]byte{}, errors.Errorf("!")},
			},
			want: nil,
		},
		{
			name: "base name exists (1)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte("python\x00/tmp/improve_system.py"), nil},
			},
			want: errors.Errorf("'improve_system.py' found in cmdline 'python /tmp/improve_system.py'"),
		},
		{
			name: "base name exists (2)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte("fw-util\x00-foobar"), nil},
			},
			want: nil,
		},
		{
			name: "base name exists (3)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte("fw-util\x00bmc\x00--update"), nil},
			},
			want: errors.Errorf("'fw-util' found in cmdline 'fw-util bmc --update'"),
		},
		{
			name: "base name exists (4)",
			readFileRet: map[string]interface{}{
				"/proc/42/cmdline": ReadFileRetType{[]byte(
					"/run/flashy/checks_and_remediations/common/00_dummy_step\x00-device\x00mtd:flash0",
				), nil},
			},
			want: errors.Errorf("'00_dummy_step' found in cmdline " +
				"'/run/flashy/checks_and_remediations/common/00_dummy_step -device mtd:flash0'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				ret := tc.readFileRet[filename].(ReadFileRetType)
				return ret.Buf, ret.Err
			}
			cmdlines := GetStringKeys(tc.readFileRet)
			got := checkNoBaseNameExistsInProcCmdlinePaths(baseNames, cmdlines)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestGetMTDInfoFromSpecifier(t *testing.T) {
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
		want            MtdInfo
		wantErr         error
	}{
		{
			name:            "wedge100 example: find mtd:flash0",
			specifier:       "flash0",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			readFileErr:     nil,
			want: MtdInfo{
				Dev:       "mtd5",
				Size:      0x02000000,
				Erasesize: 0x00010000,
			},
			wantErr: nil,
		},
		{
			name:            "ReadFile error",
			specifier:       "flash0",
			procMtdContents: "",
			readFileErr:     errors.Errorf("ReadFile error"),
			want:            MtdInfo{},
			wantErr:         errors.Errorf("Unable to read from /proc/mtd: ReadFile error"),
		},
		{
			name:            "MTD not found in /proc/mtd",
			specifier:       "flash1",
			procMtdContents: tests.ExampleWedge100ProcMtdFile,
			readFileErr:     nil,
			want:            MtdInfo{},
			wantErr:         errors.Errorf("Error finding MTD entry in /proc/mtd for flash device 'mtd:flash1'"),
		},
		{
			name:            "corrupt /proc/mtd file",
			specifier:       "flash1",
			procMtdContents: `mtd0: xxxxxxxx xxxxxxxx "flash1"`,
			readFileErr:     nil,
			want:            MtdInfo{},
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
			got, err := GetMTDInfoFromSpecifier(tc.specifier)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestIsDataPartitionMounted(t *testing.T) {
	readFileOrig := fileutils.ReadFile
	defer func() {
		fileutils.ReadFile = readFileOrig
	}()

	cases := []struct {
		name              string
		procMountContents string
		procMountReadErr  error
		want              bool
		wantErr           error
	}{
		{
			name:              "data partition exists",
			procMountContents: tests.ExampleWedge100ProcMountsFile,
			procMountReadErr:  nil,
			want:              true,
			wantErr:           nil,
		},
		{
			name:              "data partition does not exist",
			procMountContents: tests.ExampleWedge100ProcMountsFileUnmountedData,
			procMountReadErr:  nil,
			want:              false,
			wantErr:           nil,
		},
		{
			name:              "file read error",
			procMountContents: "",
			procMountReadErr:  errors.Errorf("file read error"),
			want:              false,
			wantErr:           errors.Errorf("Cannot read /proc/mounts: file read error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.ReadFile = func(filename string) ([]byte, error) {
				if filename != "/proc/mounts" {
					t.Errorf("filename: want '%v' got '%v'",
						"/proc/mounts", filename)
				}
				return []byte(tc.procMountContents), tc.procMountReadErr
			}

			got, err := IsDataPartitionMounted()
			tests.CompareTestErrors(tc.wantErr, err, t)
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}
