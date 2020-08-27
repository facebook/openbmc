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
package common

import (
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/logger"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestUnmountDataPartition(t *testing.T) {
	isDataPartitionMountedOrig := isDataPartitionMounted
	runDataPartitionUnmountProcessOrig := runDataPartitionUnmountProcess
	defer func() {
		isDataPartitionMounted = isDataPartitionMountedOrig
		runDataPartitionUnmountProcess = runDataPartitionUnmountProcessOrig
	}()

	cases := []struct {
		name           string
		dataPartExists bool
		dataPartErr    error
		unmountErr     error
		want           step.StepExitError
	}{
		{
			name:           "data part exists and successfully unmounted",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     nil,
			want:           nil,
		},
		{
			name:           "data part does not exist",
			dataPartExists: false,
			dataPartErr:    nil,
			unmountErr:     nil,
			want:           nil,
		},
		{
			name:           "data part check failed",
			dataPartExists: false,
			dataPartErr:    errors.Errorf("check failed"),
			unmountErr:     nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Unable to determine whether /mnt/data is mounted: check failed"),
			},
		},
		{
			name:           "unmount failed",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     errors.Errorf("unmount failed"),
			want: step.ExitSafeToReboot{
				errors.Errorf("Failed to unmount /mnt/data: unmount failed"),
			},
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			umountCalled := false
			isDataPartitionMounted = func() (bool, error) {
				return tc.dataPartExists, tc.dataPartErr
			}
			runDataPartitionUnmountProcess = func() error {
				umountCalled = true
				return tc.unmountErr
			}

			got := unmountDataPartition(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
			if umountCalled != tc.dataPartExists {
				t.Errorf("unmount called: want '%v' got '%v'",
					tc.want, umountCalled)
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

			got, err := isDataPartitionMounted()
			tests.CompareTestErrors(tc.wantErr, err, t)
			if tc.want != got {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestRunDataPartitionUnmountProcess(t *testing.T) {
	runCommandOrig := utils.RunCommand
	runCommandRetryOrig := utils.RunCommandWithRetries
	startSyslogOrig := logger.StartSyslog
	sleepOrig := utils.Sleep
	defer func() {
		utils.RunCommand = runCommandOrig
		utils.RunCommandWithRetries = runCommandRetryOrig
		logger.StartSyslog = startSyslogOrig
		utils.Sleep = sleepOrig
	}()

	logger.StartSyslog = func() {}
	utils.Sleep = func(t time.Duration) {}

	cases := []struct {
		name string
		// map from arg[0] of command to error
		// if not present, nil assumed
		cmdErrs  map[string]error
		wantCmds []string
		want     error
	}{
		{
			name:    "succeeded",
			cmdErrs: map[string]error{},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
				"cp -r /mnt/data /tmp/mnt",
				"fuser -km /mnt/data",
				"umount /mnt/data",
				"bash -c sshd -T 1> /dev/null",
			},
			want: nil,
		},
		{
			name: "sshd error",
			cmdErrs: map[string]error{
				"bash": errors.Errorf("sshd failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
				"cp -r /mnt/data /tmp/mnt",
				"fuser -km /mnt/data",
				"umount /mnt/data",
				"bash -c sshd -T 1> /dev/null",
			},
			want: errors.Errorf("'bash -c sshd -T 1> /dev/null', failed: sshd failed, stderr: "),
		},
		{
			name: "umount error",
			cmdErrs: map[string]error{
				"umount": errors.Errorf("umount failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
				"cp -r /mnt/data /tmp/mnt",
				"fuser -km /mnt/data",
				"umount /mnt/data",
			},
			want: errors.Errorf("'umount /mnt/data', failed: umount failed, stderr: "),
		},
		{
			name: "fuser error",
			cmdErrs: map[string]error{
				"fuser": errors.Errorf("fuser failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
				"cp -r /mnt/data /tmp/mnt",
				"fuser -km /mnt/data",
				"umount /mnt/data",
				"bash -c sshd -T 1> /dev/null",
			},
			want: nil,
		},
		{
			name: "cp error",
			cmdErrs: map[string]error{
				"cp": errors.Errorf("cp failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
				"cp -r /mnt/data /tmp/mnt",
			},
			want: errors.Errorf("'cp -r /mnt/data /tmp/mnt', failed: cp failed, stderr: "),
		},
		{
			name: "mount error",
			cmdErrs: map[string]error{
				"mount": errors.Errorf("mount failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mount --bind /mnt /tmp/mnt",
			},
			want: errors.Errorf("'mount --bind /mnt /tmp/mnt' failed: mount failed, stderr: "),
		},
		{
			name: "mkdir error",
			cmdErrs: map[string]error{
				"mkdir": errors.Errorf("mkdir failed"),
			},
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
			},
			want: errors.Errorf("'mkdir -p /tmp/mnt' failed: mkdir failed, stderr: "),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			gotCmds := []string{}
			var cmdHelper = func(cmdArr []string) (int, error, string, string) {
				gotCmds = append(gotCmds, strings.Join(cmdArr, " "))
				var errRet error
				if err, ok := tc.cmdErrs[cmdArr[0]]; ok {
					errRet = err
				}
				exitCode := 0
				if errRet != nil {
					exitCode = 1
				}
				return exitCode, errRet, "", ""
			}

			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				return cmdHelper(cmdArr)
			}

			utils.RunCommandWithRetries = func(cmdArr []string, timeout time.Duration, maxAttempts int, interval time.Duration) (int, error, string, string) {
				return cmdHelper(cmdArr)
			}

			got := runDataPartitionUnmountProcess()
			tests.CompareTestErrors(tc.want, got, t)
			if !reflect.DeepEqual(tc.wantCmds, gotCmds) {
				t.Errorf("cmds: want '%v' got '%v'", tc.wantCmds, gotCmds)
			}
		})
	}
}
