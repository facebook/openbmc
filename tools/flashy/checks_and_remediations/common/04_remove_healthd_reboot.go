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
)

func init() {
	step.RegisterStep(removeHealthdReboot)
}

// remove the high mem utilisation reboot action in /etc/healthd-config.json
// to prevent reboots mid-flash
func removeHealthdReboot(stepParams step.StepParams) step.StepExitError {
	if utils.HealthdExists() {
		log.Printf("Healthd exists in this system, removing the high memory utilization " +
			"\"reboot\" entry if it exists.")
		healthdConfig, err := utils.GetHealthdConfig()
		if err != nil {
			return step.ExitSafeToReboot{err}
		}
		err = utils.HealthdRemoveMemUtilRebootEntryIfExists(healthdConfig)
		if err != nil {
			return step.ExitSafeToReboot{err}
		}
	} else {
		log.Printf("Healthd does not exist in this system. Skipping step.")
	}
	return nil
}
