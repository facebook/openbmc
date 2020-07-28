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

package flash_procedures

import (
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func FlashCp(stepParams utils.StepParams) utils.StepExitError {
	log.Printf("Flashing using flashcp method")
	log.Printf("Attempting to flash '%v' with image file '%v'",
		stepParams.DeviceID, stepParams.ImageFilePath)

	log.Printf("Getting flash target device")
	flashDevice, err := flashutils.GetFlashDevice(stepParams.DeviceID)
	if err != nil {
		log.Printf(err.Error())
		return utils.ExitSafeToReboot{err}
	}
	log.Printf("Flash device: %v", flashDevice)

	// run flashcp
	flashCpErr := runFlashCpCmd(stepParams.ImageFilePath, flashDevice.GetFilePath())
	return flashCpErr
}

var runFlashCpCmd = func(imageFilePath, flashDevicePath string) utils.StepExitError {
	flashCmd := []string{
		"flashcp", imageFilePath, flashDevicePath,
	}

	// timeout in 30 minutes
	exitCode, err, stdout, stderr := utils.RunCommand(flashCmd, 1800)
	if err != nil {
		errMsg := fmt.Sprintf(
			"Flashing failed with exit code %v, error: %v, stdout: '%v', stderr: '%v'",
			exitCode, err, stdout, stderr,
		)
		log.Printf(errMsg)
		return utils.ExitSafeToReboot{errors.Errorf(errMsg)}
	}

	log.Printf("Flashing succeeded, safe to reboot")
	return nil
}
