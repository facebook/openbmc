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
	"syscall"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/logger"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestUnmountDataPartition(t *testing.T) {
	isDataPartitionMountedOrig := utils.IsDataPartitionMounted
	startSyslogOrig := logger.StartSyslog
	runDataPartitionUnmountProcessOrig := runDataPartitionUnmountProcess
	remountRODataPartitionOrig := remountRODataPartition
	validateSshdConfigOrig := validateSshdConfig
	defer func() {
		utils.IsDataPartitionMounted = isDataPartitionMountedOrig
		logger.StartSyslog = startSyslogOrig
		runDataPartitionUnmountProcess = runDataPartitionUnmountProcessOrig
		remountRODataPartition = remountRODataPartitionOrig
		validateSshdConfig = validateSshdConfigOrig
	}()

	logger.StartSyslog = func() {}

	cases := []struct {
		name           string
		dataPartExists bool
		dataPartErr    error
		unmountErr     error
		remountErr     error
		sshdConfigErr  error
		want           step.StepExitError
	}{
		{
			name:           "data part exists and successfully unmounted",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "data part does not exist",
			dataPartExists: false,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "data part check failed",
			dataPartExists: false,
			dataPartErr:    errors.Errorf("check failed"),
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Unable to determine whether /mnt/data is mounted: check failed"),
			},
		},
		{
			name:           "unmount failed, remount passed",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     errors.Errorf("unmount failed"),
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "unmount failed, remount failed",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     errors.Errorf("unmount failed"),
			remountErr:     errors.Errorf("remount failed"),
			sshdConfigErr:  nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Failed to unmount or remount /mnt/data"),
			},
		},
		{
			name:           "successfully unmounted but sshd config corrupt",
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  errors.Errorf("sshd config corrupt"),
			want: step.ExitUnsafeToReboot{
				errors.Errorf("Validate sshd config failed: %v",
					"sshd config corrupt"),
			},
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			umountCalled := false
			remountCalled := false
			utils.IsDataPartitionMounted = func() (bool, error) {
				return tc.dataPartExists, tc.dataPartErr
			}
			runDataPartitionUnmountProcess = func() error {
				umountCalled = true
				return tc.unmountErr
			}
			remountRODataPartition = func() error {
				remountCalled = true
				return tc.remountErr
			}
			validateSshdConfig = func() error {
				return tc.sshdConfigErr
			}

			got := unmountDataPartition(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
			if umountCalled != tc.dataPartExists {
				t.Errorf("unmount called: want '%v' got '%v'",
					tc.want, umountCalled)
			}
			if tc.dataPartExists && tc.unmountErr != nil {
				if !remountCalled {
					t.Errorf("remount called: want '%v' got '%v'",
						true, remountCalled)
				}
			}
		})
	}
}

func TestRunDataPartitionUnmountProcess(t *testing.T) {
	runCommandOrig := utils.RunCommand
	mountOrig := mount
	sleepOrig := utils.Sleep
	killDataPartitionProcessesOrig := killDataPartitionProcesses
	defer func() {
		utils.RunCommand = runCommandOrig
		mount = mountOrig
		utils.Sleep = sleepOrig
		killDataPartitionProcesses = killDataPartitionProcessesOrig
	}()

	utils.Sleep = func(t time.Duration) {}
	killDataPartitionProcesses = func() {}

	cases := []struct {
		name string
		// map from arg[0] of command to error
		// if not present, nil assumed
		cmdErrs  map[string]error
		mountErr error
		wantCmds []string
		want     error
	}{
		{
			name:     "succeeded",
			cmdErrs:  map[string]error{},
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"umount /mnt/data",
			},
			want: nil,
		},
		{
			name: "umount error",
			cmdErrs: map[string]error{
				"umount": errors.Errorf("umount failed"),
			},
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
				"umount /mnt/data",
			},
			want: errors.Errorf("umount failed"),
		},
		{
			name: "cp error",
			cmdErrs: map[string]error{
				"cp": errors.Errorf("cp failed"),
			},
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
			},
			want: errors.Errorf("Copying /mnt/data/etc/ssh to /tmp/mnt/data/etc failed: cp failed, stderr: "),
		},
		{
			name: "mkdir error",
			cmdErrs: map[string]error{
				"mkdir": errors.Errorf("mkdir failed"),
			},
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
			},
			want: errors.Errorf("'mkdir -p /tmp/mnt' failed: mkdir failed, stderr: "),
		},
		{
			name:     "mount error",
			cmdErrs:  map[string]error{},
			mountErr: errors.Errorf("Bind mount failed"),
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
			},
			want: errors.Errorf("Bind mount /mnt to /tmp/mnt failed: %v",
				"Bind mount failed"),
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

			mount = func(source string, target string, fstype string, flags uintptr, data string) (err error) {
				if source != "/mnt" {
					t.Errorf("source: want '%v' got '%v'", "/mnt", source)
				}
				if target != "/tmp/mnt" {
					t.Errorf("source: want '%v' got '%v'", "/tmp/mnt", target)
				}
				if flags != syscall.MS_BIND {
					t.Errorf("flags: want '%v' got '%v'", syscall.MS_BIND, flags)
				}
				return tc.mountErr
			}

			got := runDataPartitionUnmountProcess()
			tests.CompareTestErrors(tc.want, got, t)
			if !reflect.DeepEqual(tc.wantCmds, gotCmds) {
				t.Errorf("cmds: want '%v' got '%v'", tc.wantCmds, gotCmds)
			}
		})
	}
}

func TestValidateSshdConfig(t *testing.T) {
	runCommandOrig := utils.RunCommand
	defer func() {
		utils.RunCommand = runCommandOrig
	}()

	const sshdCmd = "bash -c sshd -T 1> /dev/null"

	cases := []struct {
		name   string
		cmdErr error
		want   error
	}{
		{
			name:   "ok",
			cmdErr: nil,
			want:   nil,
		},
		{
			name:   "failed",
			cmdErr: errors.Errorf("command failed"),
			want:   errors.Errorf("'%v' failed: command failed, stderr: ", sshdCmd),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if gotCmd != sshdCmd {
					t.Errorf("cmd: want '%v' got '%v'", sshdCmd, gotCmd)
				}
				return 0, tc.cmdErr, "", ""
			}
			got := validateSshdConfig()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestKillDataPartitionProcesses(t *testing.T) {
	runCommandOrig := utils.RunCommand
	sleepOrig := utils.Sleep
	defer func() {
		utils.RunCommand = runCommandOrig
		utils.Sleep = sleepOrig
	}()

	const fuserCmd = "fuser -km /mnt/data"

	cases := []struct {
		name   string
		cmdErr error
	}{
		{
			name:   "ok",
			cmdErr: nil,
		},
		{
			name:   "failed",
			cmdErr: errors.Errorf("command failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			sleepCalled := false
			utils.Sleep = func(t time.Duration) {
				sleepCalled = true
			}

			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if gotCmd != fuserCmd {
					t.Errorf("cmd: want '%v' got '%v'", fuserCmd, gotCmd)
				}
				return 0, tc.cmdErr, "", ""
			}

			wantSleepCalled := (tc.cmdErr == nil)

			killDataPartitionProcesses()

			if sleepCalled != wantSleepCalled {
				t.Errorf("sleep called: want '%v' got '%v'",
					wantSleepCalled, sleepCalled)
			}
		})
	}
}

func TestRemountRODataPartition(t *testing.T) {
	runCommandOrig := utils.RunCommand
	killDataPartitionProcessesOrig := killDataPartitionProcesses
	defer func() {
		utils.RunCommand = runCommandOrig
		killDataPartitionProcesses = killDataPartitionProcessesOrig
	}()

	killDataPartitionProcesses = func() {}

	const remountCmd = "mount -o remount,ro /mnt/data"

	cases := []struct {
		name   string
		cmdErr error
		want   error
	}{
		{
			name:   "ok",
			cmdErr: nil,
			want:   nil,
		},
		{
			name:   "failed",
			cmdErr: errors.Errorf("command failed"),
			want:   errors.Errorf("'%v' failed: command failed, stderr: ", remountCmd),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmd := strings.Join(cmdArr, " ")
				if gotCmd != remountCmd {
					t.Errorf("cmd: want '%v' got '%v'", remountCmd, gotCmd)
				}
				return 0, tc.cmdErr, "", ""
			}
			got := remountRODataPartition()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
