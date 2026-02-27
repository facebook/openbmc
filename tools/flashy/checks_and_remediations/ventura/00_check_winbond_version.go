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

package remediations_ventura

import (
	"fmt"
	"log"
	"strconv"
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func init() {
	step.RegisterStep(checkWinbondVersion)
}

// Related to S483584
func checkWinbondVersion(stepParams step.StepParams) step.StepExitError {

	// First check if Winbond manufacturer
	manufacturer, err := utils.GetBSMFlashManufacturerFromFile()
	if err != nil {
		errMsg := fmt.Errorf("Unable to fetch SPI Manufacturer info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}
	if manufacturer != "winbond" {
		log.Printf("Not a winbond SPI chip. Unaffected by S483584")
		return nil
	}

	version, err := utils.GetOpenBMCVersionFromIssueFile()
	if err != nil {
		errMsg := fmt.Errorf("Unable to fetch version info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	// Check if BSM version has fix for BSMs with Winbond SPI chips
	const re = `ventura-v(?P<year>[0-9]{4}).(?P<week>[0-9]{2}).(?P<revision>[0-9]+)`
	versionMap, err := utils.GetRegexSubexpMap(re, version)
	if err != nil {
		if strings.HasPrefix(version, "ventura-") {
			log.Printf("Upgrading from dirty version. Skipping year check")
			return nil
		} else {
			errMsg := fmt.Errorf("Unable to parse version info: %v", err)
			return step.ExitUnsafeToReboot{Err: errMsg}
		}
	}

	year, _ := strconv.Atoi(versionMap["year"])
	week, _ := strconv.Atoi(versionMap["week"])
	revision, _ := strconv.Atoi(versionMap["revision"])

	// Fix is applied from 2025.02.02 =>
	if year < 2025 || (year == 2025 && (week < 2 || week == 2 && revision < 2)) {
		errMsg := fmt.Errorf("Cannot upgrade this version (%v) due to S483584", version)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	log.Printf("Version %v contains Winbond fix for S483584. Safe to upgrade", version)
	return nil
}
