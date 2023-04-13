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
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashcp"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

// FlashCp is a step function that runs the flashcp procedure.
func FlashCp(stepParams step.StepParams) step.StepExitError {
	log.Printf("Flashing using flashcp method")
	log.Printf("Attempting to flash '%v' with image file '%v'",
		stepParams.DeviceID, stepParams.ImageFilePath)

	log.Printf("Getting flash target device")
	flashDevice, err := flashutils.GetFlashDevice(stepParams.DeviceID)
	if err != nil {
		log.Print(err.Error())
		return step.ExitSafeToReboot{Err: err}
	}
	log.Printf("Flash device: %v", flashDevice)
	err = flashCpAndValidate(flashDevice, stepParams.ImageFilePath, 0)
	if err != nil {
		// failed validation considered to be a deal breaker
		return step.ExitUnsafeToReboot{Err: err}
	}

	// make sure no other flasher is running
	flashyStepBaseNames := step.GetFlashyStepBaseNames()
	err = utils.CheckOtherFlasherRunning(flashyStepBaseNames)
	if err != nil {
		log.Printf("Flashing succeeded but found another flasher running: %v", err)
		return step.ExitUnsafeToReboot{Err: err}
	}

	return nil
}

// flashCpAndValidate calls Flashy's internal implementation of FlashCp.
// roOffset is the starting RO offset for FlashCp. It's used for vboot
// flash devices with RO offsets. (In `dd` terms, roOffset is both seek=
// and skip=).
var flashCpAndValidate = func(
	flashDevice devices.FlashDevice,
	imageFilePath string,
	roOffset uint32,
) error {
	log.Printf("Starting to flash")
	err := flashcp.FlashCp(imageFilePath, flashDevice.GetFilePath(), roOffset)
	if err != nil {
		log.Printf("FlashCp failed: %v", err)
	} else {
		log.Printf("FlashCp succeeded")
	}
	// reset motd back to default on LF OpenBMC
	_ = fileutils.RemoveFile("/run/mnt-persist/etc-data/motd")
	return err
}
