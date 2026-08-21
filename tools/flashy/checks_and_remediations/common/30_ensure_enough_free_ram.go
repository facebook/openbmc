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
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func init() {
	step.RegisterStep(ensureEnoughFreeRAM)
}

// This is 75% of the limit in pypartition, as flashy assumes
// the image is already downloaded.
// This should be a generous limit to allow flashy and flashcp to run.
const minMemoryNeeded = 45 * 1024 * 1024

// S697061: wedge100 keeps a far larger rootfs pinned in unevictable ramfs than the platforms
// this check was originally sized against, so it sits below the default limit on all
// but a freshly booted device and ends up asking for a reboot on every upgrade.
const wedge100MinMemoryNeeded = 30 * 1024 * 1024

func ensureEnoughFreeRAM(stepParams step.StepParams) step.StepExitError {
	memInfo, err := utils.GetMemInfo()
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}

	platform, err := utils.GetOpenBMCPlatformFromIssueFile()
	if err != nil {
		return step.ExitSafeToReboot{Err: fmt.Errorf("Unable to determine platform: %w", err)}
	}

	// S697061: Newer wedge100 versions have a much larger rootfs
	minMemory := uint64(minMemoryNeeded)
	if platform == "wedge100" {
		minMemory = wedge100MinMemoryNeeded
	}

	log.Printf("Memory status: %v B total memory, %v B free memory", memInfo.MemTotal, memInfo.MemFree)
	log.Printf("Minimum memory needed for update is %v B", minMemory)

	if memInfo.MemFree < minMemory {
		errMsg := fmt.Errorf("Free memory (%v B) < minimum memory needed (%v B), reboot needed",
			memInfo.MemFree, minMemory)
		return step.ExitSafeToReboot{Err: errMsg}
	}

	return nil
}
