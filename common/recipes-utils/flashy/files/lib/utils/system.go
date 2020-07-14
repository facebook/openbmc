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
	"bufio"
	"context"
	"fmt"
	"log"
	"os/exec"
	"regexp"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/pkg/errors"
)

// memory information in bytes
type MemInfo struct {
	MemTotal uint64
	MemFree  uint64
}

// get memInfo
// note that this assumes kB units for MemFree and MemTotal
// it will fail otherwise
var GetMemInfo = func() (*MemInfo, error) {
	buf, err := ReadFile("/proc/meminfo")
	if err != nil {
		return nil, errors.Errorf("Unable to open /proc/meminfo: %v", err)
	}

	memInfoStr := string(buf)

	memFreeRegex := regexp.MustCompile(`(?m)^MemFree: +([0-9]+) kB$`)
	memTotalRegex := regexp.MustCompile(`(?m)^MemTotal: +([0-9]+) kB$`)

	memFreeMatch := memFreeRegex.FindStringSubmatch(memInfoStr)
	if len(memFreeMatch) < 2 {
		return nil, errors.Errorf("Unable to get MemFree in /proc/meminfo")
	}
	memFree, _ := strconv.ParseUint(memFreeMatch[1], 10, 64)
	memFree *= 1024 // convert to B

	memTotalMatch := memTotalRegex.FindStringSubmatch(memInfoStr)
	if len(memTotalMatch) < 2 {
		return nil, errors.Errorf("Unable to get MemTotal in /proc/meminfo")
	}
	memTotal, _ := strconv.ParseUint(memTotalMatch[1], 10, 64)
	memTotal *= 1024 // convert to B

	return &MemInfo{
		memTotal,
		memFree,
	}, nil
}

// function to aid logging and saving live stdout and stderr output
// from running command
// note that sequential execution is not guaranteed - race conditions
// might still exist
func logScanner(s *bufio.Scanner, ch chan struct{}, pre string, str *string) {
	for s.Scan() {
		t := fmt.Sprintf("%v\n", s.Text())
		log.Printf("%v%v", pre, t)
		*str = *str + t
	}
	close(ch)
}

// runs command and pipes live output
// returns exitcode, error, stdout (string), stderr (string) if non-zero/error returned or timed out
// returns 0, nil, stdout (string), stderr (string) if successfully run
var RunCommand = func(cmdArr []string, timeoutInSeconds int) (int, error, string, string) {
	start := time.Now()
	timeout := time.Duration(timeoutInSeconds) * time.Second
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	fullCmdStr := strings.Join(cmdArr[:], " ")
	log.Printf("Running command '%v' with %v timeout", fullCmdStr, timeout)
	cmd := exec.CommandContext(ctx, cmdArr[0], cmdArr[1:]...)

	var stdoutStr, stderrStr string
	stdout, _ := cmd.StdoutPipe()
	stderr, _ := cmd.StderrPipe()
	stdoutScanner := bufio.NewScanner(stdout)
	stderrScanner := bufio.NewScanner(stderr)
	stdoutDone := make(chan struct{})
	stderrDone := make(chan struct{})
	go logScanner(stdoutScanner, stdoutDone, "stdout: ", &stdoutStr)
	go logScanner(stderrScanner, stderrDone, "stderr: ", &stderrStr)

	exitCode := 1 // show 1 if failed by default

	if err := cmd.Start(); err != nil {
		log.Printf("Command '%v' failed to start: %v", fullCmdStr, err)
		// failed to start, exit now
		return exitCode, err, stdoutStr, stderrStr
	}

	err := cmd.Wait()
	if err != nil {
		if exitErr, ok := err.(*exec.ExitError); ok {
			// The program exited with exit code != 0
			waitStatus := exitErr.Sys().(syscall.WaitStatus)
			exitCode = waitStatus.ExitStatus()
		} else {
			log.Printf("Could not get exit code, defaulting to '1'")
		}
	} else {
		// exit code should be 0
		waitStatus := cmd.ProcessState.Sys().(syscall.WaitStatus)
		exitCode = waitStatus.ExitStatus()
	}

	<-stdoutDone
	<-stderrDone

	elapsed := time.Since(start)

	if ctx.Err() == context.DeadlineExceeded {
		log.Printf("Command '%v' timed out after %v", fullCmdStr, timeout)
		// replace the err with the deadline exceeded error
		// instead of just signal: killed
		err = ctx.Err()
	} else {
		log.Printf("Command '%v' exited with code %v after %v", fullCmdStr, exitCode, elapsed)
	}

	return exitCode, err, stdoutStr, stderrStr
}

// check whether systemd is available
var SystemdAvailable = func() (bool, error) {
	const cmdlinePath = "/proc/1/cmdline"

	if FileExists(cmdlinePath) {
		buf, err := ReadFile(cmdlinePath)
		if err != nil {
			return false, errors.Errorf("%v exists but cannot be read: %v", cmdlinePath, err)
		}

		contains := strings.Contains(string(buf), "systemd")

		return contains, nil
	}

	return false, nil
}
