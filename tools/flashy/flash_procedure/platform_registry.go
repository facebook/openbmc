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

package flash_procedure

import (
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/flash"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
)

var procedureOverrides = map[string]func(step.StepParams) step.StepExitError{
	"fby3": flash.Yv3Flash,
}

var FlashProcedureMappings = map[string]func(step.StepParams) step.StepExitError{}

func init() {
	for platformName, flashFunc := range GeneratedFlashProcedureMappings {
		FlashProcedureMappings[platformName] = flashFunc
	}

	for platformName, overrideFunc := range procedureOverrides {
		if _, exists := GeneratedFlashProcedureMappings[platformName]; exists {
			FlashProcedureMappings[platformName] = overrideFunc
		}
	}

	for platformName, compatiblePlatformName := range validate.CompatibleVersionMapping {
		_, exists := FlashProcedureMappings[platformName]
		if exists {
			// prevent overriding existing mappings (eg: in case any override)
			continue
		}

		_, exists = FlashProcedureMappings[compatiblePlatformName]
		if !exists {
			log.Printf("WARNING: compatible platform name '%s' not present in flash procedure mapping", compatiblePlatformName)
			continue
		}

		FlashProcedureMappings[platformName] = FlashProcedureMappings[compatiblePlatformName]
	}
}

func PlatformExists(platformName string) bool {
	_, exists := FlashProcedureMappings[platformName]
	return exists
}

func ExecuteFlashProcedure(platformName string, stepParams step.StepParams) step.StepExitError {
	flashFunc, ok := FlashProcedureMappings[platformName]
	if !ok {
		return step.ExitSafeToReboot{
			Err: fmt.Errorf("unknown platform '%s' - no flash procedure found", platformName),
		}
	}

	return flashFunc(stepParams)
}
