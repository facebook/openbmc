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

 package remediations_yosemite4

 import (
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"log"
	"testing"
	"bytes"
	"os"
	"regexp"
	"strings"
 )

 func TestKillMfgToolProcesses(t *testing.T) {
	// save log output into buf for testing
	var logBuffer bytes.Buffer
	log.SetOutput(&logBuffer)

	// mock cleanup
	realListProcessesMatchingRegex := utils.ListProcessesMatchingRegex
	realKill := kill

	defer func() {
		log.SetOutput(os.Stderr)
		kill = realKill
		utils.ListProcessesMatchingRegex = realListProcessesMatchingRegex
	}()

	t.Run("Must not kill mfg-tool if there's a single process", func(t *testing.T) {
		logBuffer.Reset()

		// Simulate a single mfg-tool process
		utils.ListProcessesMatchingRegex = func(regex *regexp.Regexp) ([]*os.Process, error) {
			return []*os.Process{&os.Process{Pid: 1234}}, nil
		}

		// Mock kill
		killCalled := false
		kill = func (proc *os.Process) error {
			killCalled = true
			return nil;
		}

		res := killMfgToolProcesses(step.StepParams{})

		if res != nil {
			t.Errorf("Expected killMfgToolProcesses() to return nil, got %v", res)
		}

		if !strings.Contains(logBuffer.String(), "Found 1 mfg-tool running processes") {
			t.Errorf("Expected 'Found 1 mfg-tool running processes' log message, got %v", logBuffer.String())
		}

		if killCalled {
			t.Errorf("Expected killMfgToolProcesses() to not call kill() if there's a single mfg-tool process")
		}
	})

	t.Run("Must kill all mfg-tool if there are multiple mfg-tool processes", func(t *testing.T) {
		logBuffer.Reset()

		// Simulate multiple mfg-tool processes
		utils.ListProcessesMatchingRegex = func(regex *regexp.Regexp) ([]*os.Process, error) {
			return []*os.Process{&os.Process{Pid: 1234}, &os.Process{Pid: 4567}}, nil
		}

		// Mock kill
		killCalled := false
		kill = func (proc *os.Process) error {
			killCalled = true
			return nil;
		}

		res := killMfgToolProcesses(step.StepParams{})

		if res != nil {
			t.Errorf("Expected killMfgToolProcesses() to return nil, got %v", res)
		}

		if !strings.Contains(logBuffer.String(), "Found 2 mfg-tool running processes") {
			t.Errorf("Expected 'Found 2 mfg-tool running processes' log message, got %v", logBuffer.String())
		}

		// Ensure all mfg-tool processes were killed
		if !strings.Contains(logBuffer.String(), "Killing mfg-tool process with pid=1234") || !strings.Contains(logBuffer.String(), "Killing mfg-tool process with pid=4567") {
			t.Errorf("Expected logs showing all mfg-tool processes being killed got %v", logBuffer.String())
		}

		if !killCalled {
			t.Errorf("Expected killMfgToolProcesses() to call kill() on multiple mfg-tool processes")
		}
	})


}
