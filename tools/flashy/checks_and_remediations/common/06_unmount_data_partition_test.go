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
	isLfOrig := utils.IsLFOpenBMC
	defer func() {
		utils.IsDataPartitionMounted = isDataPartitionMountedOrig
		logger.StartSyslog = startSyslogOrig
		runDataPartitionUnmountProcess = runDataPartitionUnmountProcessOrig
		remountRODataPartition = remountRODataPartitionOrig
		validateSshdConfig = validateSshdConfigOrig
		utils.IsLFOpenBMC = isLfOrig
	}()

	logger.StartSyslog = func() {}

	cases := []struct {
		name           string
		isLFOpenBMC    bool
		dataPartExists bool
		dataPartErr    error
		unmountErr     error
		remountErr     error
		sshdConfigErr  error
		want           step.StepExitError
	}{
		{
			name:           "data part exists and successfully unmounted",
			isLFOpenBMC:    false,
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "data part does not exist",
			isLFOpenBMC:    false,
			dataPartExists: false,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "data part check failed",
			isLFOpenBMC:    false,
			dataPartExists: false,
			dataPartErr:    errors.Errorf("check failed"),
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Unable to determine whether /mnt/data is mounted: check failed"),
			},
		},
		{
			name:           "unmount failed, remount passed",
			isLFOpenBMC:    false,
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     errors.Errorf("unmount failed"),
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
		{
			name:           "unmount failed, remount failed",
			isLFOpenBMC:    false,
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     errors.Errorf("unmount failed"),
			remountErr:     errors.Errorf("remount failed"),
			sshdConfigErr:  nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Failed to unmount or remount /mnt/data"),
			},
		},
		{
			name:           "successfully unmounted but sshd config corrupt",
			isLFOpenBMC:    false,
			dataPartExists: true,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  errors.Errorf("sshd config corrupt"),
			want: step.ExitUnsafeToReboot{
				Err: errors.Errorf("Validate sshd config failed: %v",
					"sshd config corrupt"),
			},
		},
		{
			name:           "LF OpenBMC, do nothing",
			isLFOpenBMC:    true,
			dataPartExists: false,
			dataPartErr:    nil,
			unmountErr:     nil,
			remountErr:     nil,
			sshdConfigErr:  nil,
			want:           nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			umountCalled := false
			remountCalled := false
			utils.IsLFOpenBMC = func() (bool) {
				return tc.isLFOpenBMC
			}
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
		stderr string
		mountErr error
		wantCmds []string
		want     error
	}{
		{
			name:     "succeeded",
			cmdErrs:  map[string]error{},
			stderr: "",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store",
				"umount /mnt/data",
			},
			want: nil,
		},
		{
			name: "umount error",
			cmdErrs: map[string]error{
				"umount /mnt/data": errors.Errorf("umount failed"),
			},
			stderr: "",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store",
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
			name: "cp ssh error",
			cmdErrs: map[string]error{
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc": errors.Errorf("cp failed"),
			},
			stderr: "",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
			},
			want: errors.Errorf("Copying /mnt/data/etc/ssh to /tmp/mnt/data/etc failed: cp failed, stderr: "),
		},
		{
			name: "cp kv_store error",
			cmdErrs: map[string]error{
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store": errors.Errorf("cp failed"),
			},
			stderr: "oops",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store",
			},
			want: errors.Errorf("Copying /mnt/data/kv_store to /tmp/mnt/data/kv_store failed: cp failed, stderr: oops"),
		},
		{
			name: "cp kv_store allowable error",
			cmdErrs: map[string]error{
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store": errors.Errorf("cp failed"),
			},
			stderr: "Error: No such file or directory",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
				"mkdir -p /tmp/mnt/data/etc",
				"cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc",
				"cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store",
				"umount /mnt/data",
			},
			want: nil,
		},
		{
			name: "mkdir error",
			cmdErrs: map[string]error{
				"mkdir -p /tmp/mnt": errors.Errorf("mkdir failed"),
			},
			stderr: "",
			mountErr: nil,
			wantCmds: []string{
				"mkdir -p /tmp/mnt",
			},
			want: errors.Errorf("'mkdir -p /tmp/mnt' failed: mkdir failed, stderr: "),
		},
		{
			name:     "mount error",
			cmdErrs:  map[string]error{},
			stderr: "",
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
				cmd := strings.Join(cmdArr, " ")
				gotCmds = append(gotCmds, cmd)
				var errRet error
				if err, ok := tc.cmdErrs[cmd]; ok {
					errRet = err
				}
				exitCode := 0
				if errRet != nil {
					exitCode = 1
				}
				return exitCode, errRet, "", tc.stderr
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
