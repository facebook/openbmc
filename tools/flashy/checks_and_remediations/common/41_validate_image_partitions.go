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
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
)

func init() {
	step.RegisterStep(validateImagePartitions)
}

func validateImagePartitions(stepParams step.StepParams) step.StepExitError {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing image partition validation =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}

	if utils.IsPfrSystem() {
		log.Printf("This is a PFR system, validation is left to fw-util.")
		return nil
	}

	err := image.ValidateImageFile(stepParams.ImageFilePath, stepParams.DeviceID)
	if err != nil {
		// the image could've been corrupted during copy,
		// it is safe to reboot.
		return step.ExitSafeToReboot{Err: err}
	}
	return nil
}
