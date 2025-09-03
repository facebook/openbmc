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

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

// Variables for testing - can be overridden in tests
var (
	flashFwUtilFunc = FlashFwUtil
	flashCpVbootFunc = FlashCpVboot
)

// Known vboot systems: fby3.
func Yv3Flash(stepParams step.StepParams) step.StepExitError {
	if utils.IsPfrSystem() {
		log.Printf("This is a PFR system: Using fw-util to flash.")
		return flashFwUtilFunc(stepParams)
	}
	log.Printf("Not a PFR system: Using flashcp vboot to flash.")
	return flashCpVbootFunc(stepParams)
}
