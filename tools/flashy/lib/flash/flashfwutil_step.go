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

package flash

import (
	"fmt"
	"log"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/kballard/go-shellquote"
	"github.com/pkg/errors"
)

// FlashFwUtil is a step function that runs the fw-util flash procedure.
// This is currently used for PFR systems.
func FlashFwUtil(stepParams step.StepParams) step.StepExitError {
	log.Printf("Flashing using fw-util method")
	log.Printf("Attempting to flash with image file '%v'", stepParams.ImageFilePath)

	err := runFwUtilCmd(stepParams.ImageFilePath)
	if err != nil {
		return step.ExitSafeToReboot{err}
	}

	// make sure no other flasher is running
	flashyStepBaseNames := step.GetFlashyStepBaseNames()
	err = utils.CheckOtherFlasherRunning(flashyStepBaseNames)
	if err != nil {
		log.Printf("Flashing succeeded but found another flasher running: %v", err)
		return step.ExitUnsafeToReboot{err}
	}

	return nil
}

var runFwUtilCmd = func(imageFilePath string) error {
	log.Printf("Starting to run fw-util")

	// fw-util has flashcp -v which is very verbose and makes it slow,
	// redirect output to /dev/null
	fwutilCmd := shellquote.Join(
		"fw-util", "bmc", "--update", "bmc", imageFilePath,
	)

	flashCmd := []string{
		"bash", "-c", fmt.Sprintf("%v > /dev/null", fwutilCmd),
	}

	exitCode, err, stdout, stderr := utils.RunCommand(flashCmd, 1800*time.Second)
	if err != nil {
		errMsg := fmt.Sprintf(
			"Flashing failed with exit code %v, error: %v, stdout: '%v', stderr: '%v'",
			exitCode, err, stdout, stderr,
		)
		return errors.Errorf(errMsg)
	}
	log.Printf("fw-util succeeded")

	return nil
}
