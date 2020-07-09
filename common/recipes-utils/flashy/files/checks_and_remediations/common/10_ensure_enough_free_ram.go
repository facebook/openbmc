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

	"github.com/facebook/openbmc/tree/helium/common/recipes-utils/flashy/files/utils"
	"github.com/pkg/errors"
)

func init() {
	utils.RegisterStepEntryPoint(ensureEnoughFreeRam)
}

// In the case of no reboot threshold, use a default limit
// This is 75% of the limit in pypartition, as flashy assumes
// the image is already downloaded.
// When there is a reboot threshold, this is added to it.
// This should be a generous limit to allow flashy and flashcp to run.
const defaultThreshold = 45 * 1024 * 1024

func ensureEnoughFreeRam(imageFilePath, deviceID string) utils.StepExitError {
	memInfo, err := utils.GetMemInfo()
	if err != nil {
		return utils.ExitSafeToReboot{err}
	}
	log.Printf("Memory status: %v B total memory, %v B free memory", memInfo.MemTotal, memInfo.MemFree)

	minMemoryNeeded, err := getMinMemoryNeeded(memInfo)
	if err != nil {
		return utils.ExitSafeToReboot{err}
	}

	log.Printf("Minimum memory needed for update is %v B", minMemoryNeeded)

	if memInfo.MemFree < minMemoryNeeded {
		errMsg := errors.Errorf("Free memory (%v B) < minimum memory needed (%v B), reboot needed",
			memInfo.MemFree, minMemoryNeeded)
		return utils.ExitSafeToReboot{errMsg}
	}

	return nil
}

// get the minimum memory (in B) required
var getMinMemoryNeeded = func(memInfo *utils.MemInfo) (uint64, error) {
	var minMemNeeded uint64

	if utils.HealthdExists() {
		log.Printf("Healthd exists: getting healthd reboot threshold.")

		healthdConfig, err := utils.GetHealthdConfig()
		if err != nil {
			return minMemNeeded, err
		}

		healthdRebootThresholdPercentage := healthdConfig.GetRebootThresholdPercentage()

		// add the default threshold to the healthd threshold
		healthdRebootThreshold :=
			uint64(((100.0-healthdRebootThresholdPercentage)/100)*float32(memInfo.MemTotal)) +
				defaultThreshold

		log.Printf("Healthd reboot threshold at %v percent",
			healthdRebootThresholdPercentage)

		log.Printf("Using healthd reboot threshold (%v B)",
			healthdRebootThreshold)

		minMemNeeded = healthdRebootThreshold
	} else {
		log.Printf("Healthd does not exist in this system.")
		log.Printf("Using default threshold (%v B)", defaultThreshold)
		minMemNeeded = defaultThreshold
	}

	return minMemNeeded, nil
}
