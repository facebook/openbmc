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
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

// known vboot systems: yosemite2, brycecanyon1, tiogapass1, yosemitegpv2, northdome
// these vboot devices expose only flash1
func FlashCpVboot(stepParams step.StepParams) step.StepExitError {
	log.Printf("Flashing using flashcp vboot method")

	log.Printf("Attempting to flash '%v' with image file '%v'",
		stepParams.DeviceID, stepParams.ImageFilePath)

	log.Printf("Getting flash target device")
	flashDevice, err := flashutils.GetFlashDevice(stepParams.DeviceID)
	if err != nil {
		log.Printf(err.Error())
		return step.ExitSafeToReboot{err}
	}
	log.Printf("Flash device: %v", flashDevice)

	// ==== WARNING: THIS STEP CAN ALTER THE IMAGE FILE ====
	err = flashutils.VbootPatchImageBootloaderIfNeeded(stepParams.ImageFilePath, flashDevice)
	if err != nil {
		log.Printf(err.Error())
		return step.ExitSafeToReboot{err}
	}

	// run flashcp
	flashCpErr := runFlashCpCmd(stepParams.ImageFilePath, flashDevice.GetFilePath())
	return flashCpErr
}
