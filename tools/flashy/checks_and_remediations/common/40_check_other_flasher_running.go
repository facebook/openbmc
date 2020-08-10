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
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(checkOtherFlasherRunning)
}

// fail if any other flashers are running
func checkOtherFlasherRunning(stepParams step.StepParams) step.StepExitError {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing check for other running flashers =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}

	flashyStepBaseNames := step.GetFlashyStepBaseNames()

	err := utils.CheckOtherFlasherRunning(flashyStepBaseNames)
	if err != nil {
		return step.ExitUnsafeToReboot{
			errors.Errorf("Another flasher detected: %v. "+
				"Use the '--clowntown' flag if you wish to proceed at the risk of "+
				"bricking the device", err),
		}
	}
	return nil
}
