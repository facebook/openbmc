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

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func FlashCp(stepParams step.StepParams) step.StepExitError {
	log.Printf("Flashing using flashcp method")
	log.Printf("Attempting to flash '%v' with image file '%v'",
		stepParams.DeviceID, stepParams.ImageFilePath)

	log.Printf("Getting flash target device")
	flashDevice, err := flashutils.GetFlashDevice(stepParams.DeviceID)
	if err != nil {
		log.Printf(err.Error())
		return step.ExitSafeToReboot{err}
	}
	log.Printf("Flash device: %v", flashDevice)
	err = flashCpAndValidate(flashDevice, stepParams.ImageFilePath)
	if err != nil {
		// return safe to reboot here to retry.
		// error handler (step.HandleStepError) will check if
		// either flash device is valid.
		return step.ExitSafeToReboot{err}
	}
	return nil
}

var flashCpAndValidate = func(flashDevice devices.FlashDevice, imageFilePath string) error {
	log.Printf("Starting to flash")
	err := runFlashCpCmd(imageFilePath, flashDevice.GetFilePath())
	if err != nil {
		log.Printf("Flashcp failed: %v", err)
		return err
	}
	log.Printf("Flashcp succeeded")

	log.Printf("Validating flash device")
	err = flashDevice.Validate()
	if err != nil {
		log.Printf("Flash device validation failed: %v", err)
		return err
	}
	log.Printf("Flash device validation passed")
	return nil
}

var runFlashCpCmd = func(imageFilePath, flashDevicePath string) error {
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
		return errors.Errorf(errMsg)
	}

	return nil
}
