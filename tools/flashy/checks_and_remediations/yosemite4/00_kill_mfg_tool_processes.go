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
	"os"
	"regexp"
	"syscall"
)

func init() {
	step.RegisterStep(killMfgToolProcesses)
}

func killMfgToolProcesses(stepParams step.StepParams) step.StepExitError {
	// Find all running mfg-tool processes
	mfgToolRegex := regexp.MustCompile(`(^|/)mfg-tool(\x00|$)`)
	mfgToolProcs, err := utils.ListProcessesMatchingRegex(mfgToolRegex)
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}
	log.Printf("Found %d mfg-tool running processes", len(mfgToolProcs))

	// S458061: If there's more than one running mfg-tool process, kill them all
	// to prevent stuck mfg-tool instances causing OOM problems during flashing
	if len(mfgToolProcs) > 1 {
		for _, proc := range mfgToolProcs {
			log.Printf("Killing mfg-tool process with pid=%d", proc.Pid)
			err := kill(proc)
			if err != nil {
				log.Printf("Failed to kill mfg-tool process with pid=%d: %v", proc.Pid, err)
			}
		}
	}

	return nil
}

var kill = func(proc *os.Process) error {
	// Just a simple wrapper so it's easier to mock in tests
	return proc.Signal(syscall.SIGKILL)
}
