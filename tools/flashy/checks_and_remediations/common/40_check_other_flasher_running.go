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
	"fmt"
	"log"
	"os"
	"path"
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(checkOtherFlasherRunning)
}

// other flashers + the "flashy" binary
// flashy steps will be added in via getFlashyStepBaseNames
var otherFlasherBaseNames = []string{
	"autodump.sh",
	"cpld_upgrade.sh",
	"dd",
	"flash_eraseall",
	"flashcp",
	"flashrom",
	"fw-util",
	"fw_setenv",
	"improve_system.py",
	"jbi",
	"psu-update-bel.py",
	"psu-update-delta.py",
	"flashy",
}

var ownCmdlines = []string{
	"/proc/self/cmdline",
	"/proc/thread-self/cmdline",
	fmt.Sprintf("/proc/%v/cmdline", os.Getpid()),
}

// fail if any other flashers are running
// TODO:- when the steps are reordered, this step must be the first so that there
// are no other ExitSafeToReboot errors causing reboots!
func checkOtherFlasherRunning(stepParams step.StepParams) step.StepExitError {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing check for other running flashers =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}
	flashyStepBaseNames := getFlashyStepBaseNames()

	allFlasherBaseNames := append(otherFlasherBaseNames, flashyStepBaseNames...)
	otherProcCmdlinePaths, err := getOtherProcCmdlinePaths()
	if err != nil {
		return step.ExitUnknownError{err}
	}

	err = checkNoBaseNameExistsInProcCmdlinePaths(allFlasherBaseNames, otherProcCmdlinePaths)
	if err != nil {
		return step.ExitUnsafeToReboot{
			errors.Errorf("Another flasher detected: %v. "+
				"Use the '--clowntown' flag if you wish to proceed at the risk of "+
				"bricking the device", err),
		}
	}
	return nil
}

// return error if a basename is found running in a proc/*/cmdline file
var checkNoBaseNameExistsInProcCmdlinePaths = func(baseNames, procCmdlinePaths []string) error {
	for _, procCmdlinePath := range procCmdlinePaths {
		cmdlineBuf, err := fileutils.ReadFile(procCmdlinePath)
		if err != nil {
			// processes and their respective files in procfs naturally come and go
			continue
		}

		// Consider all command line parameters so `python improve_system.py` and
		// `improve_system.py` are both detected.
		params := strings.Split(string(cmdlineBuf), "\x00")
		for _, param := range params {
			baseName := path.Base(param)
			if utils.StringFind(baseName, baseNames) >= 0 {
				return errors.Errorf("'%v' found in cmdline '%v'",
					baseName, strings.Join(params, " "))
			}
		}

	}
	return nil
}

// get basenames of flashy's steps
var getFlashyStepBaseNames = func() []string {
	flashyStepBaseNames := []string{}
	for p, _ := range step.StepMap {
		stepBasename := path.Base(p)
		flashyStepBaseNames = append(flashyStepBaseNames, stepBasename)
	}
	return flashyStepBaseNames
}

// get all other /proc/*/cmdlines that are not in our own cmdlines (ownCmdlines)
var getOtherProcCmdlinePaths = func() ([]string, error) {
	allCmdlines, err := fileutils.Glob("/proc/*/cmdline")
	if err != nil {
		return nil, err
	}
	otherCmdlines := utils.StringDifference(allCmdlines, ownCmdlines)
	return otherCmdlines, nil
}
