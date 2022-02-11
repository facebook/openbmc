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
	step.RegisterStep(ensureEnoughFreeRAM)
}

// This is 75% of the limit in pypartition, as flashy assumes
// the image is already downloaded.
// This should be a generous limit to allow flashy and flashcp to run.
const minMemoryNeeded = 45 * 1024 * 1024

func ensureEnoughFreeRAM(stepParams step.StepParams) step.StepExitError {
	memInfo, err := utils.GetMemInfo()
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}
	log.Printf("Memory status: %v B total memory, %v B free memory", memInfo.MemTotal, memInfo.MemFree)
	log.Printf("Minimum memory needed for update is %v B", minMemoryNeeded)

	if memInfo.MemFree < minMemoryNeeded {
		errMsg := errors.Errorf("Free memory (%v B) < minimum memory needed (%v B), reboot needed",
			memInfo.MemFree, minMemoryNeeded)
		return step.ExitSafeToReboot{Err: errMsg}
	}

	return nil
}
