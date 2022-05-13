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

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

// In vboot systems, there exists a 84k RO region to skip.
const vbootRomxSize = 84 * 1024

var vbootRomxExists = func(flashDeviceSpecifier string) bool {
	// VBOOT_HARDWARE_ENFORCE means that flash0 is locked in its
	// entirety and also that the first 64kB of flash1 is locked.
	// However there is a kernel/U-Boot bug that causes detection
	// and flagging of locked chips/regions to fail sometimes.
	//
	// VBOOT_SOFTWARE_ENFORCE means that U-Boot on flash0 is verifying
	// the signature of images loaded from flash1.  However if the BMC
	// has booted in recovery mode this flag may be false.
	//
	// Therefore always assume that the start of flash1 is read-only.
	return flashDeviceSpecifier == "flash1"
}

// FlashCpVboot is a step function that runs the flashcp procedure with extra vboot
// procedures.
// Known vboot systems: yosemite2, brycecanyon1, tiogapass1, yosemitegpv2, northdome.
// These vboot devices expose only flash1.
func FlashCpVboot(stepParams step.StepParams) step.StepExitError {
	log.Printf("Flashing using flashcp vboot method")
	log.Printf("Attempting to flash '%v' with image file '%v'",
		stepParams.DeviceID, stepParams.ImageFilePath)

	log.Printf("Getting flash target device")
	flashDevice, err := flashutils.GetFlashDevice(stepParams.DeviceID)
	if err != nil {
		log.Print(err.Error())
		return step.ExitSafeToReboot{Err: err}
	}
	log.Printf("Flash device: %v", flashDevice)

	roOffset := uint32(0)
	if vbootRomxExists(flashDevice.GetSpecifier()) {
		log.Printf("vboot ROM part exists in flash device. Skipping %vB ROM region.",
			vbootRomxSize)
		roOffset = vbootRomxSize
	}

	err = flashCpAndValidate(
		flashDevice,
		stepParams.ImageFilePath,
		roOffset,
	)
	if err != nil {
		// return safe to reboot here to retry.
		// error handler (step.HandleStepError) will check if
		// either flash device is valid.
		return step.ExitSafeToReboot{Err: err}
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
