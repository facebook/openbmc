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

package remediations_bletchley

import (
	"strconv"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(checkVersion)
}

func checkVersion(stepParams step.StepParams) step.StepExitError {
	version, err := utils.GetOpenBMCVersionFromIssueFile()
	if err != nil {
		errMsg := errors.Errorf("Unable to fetch version info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	const re = `bletchley-v(?P<year>[0-9]+).(?P<week>[0-9]+)`
	versionMap, err := utils.GetRegexSubexpMap(re, version)
	if err != nil {
		errMsg := errors.Errorf("Unable to parse version info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	year, err := strconv.Atoi(versionMap["year"])
	if err != nil {
		errMsg := errors.Errorf("Unable to parse version info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	week, err := strconv.Atoi(versionMap["week"])
	if err != nil {
		errMsg := errors.Errorf("Unable to parse version info: %v", err)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}
	
	if year < 2023 || (year == 2023 && week < 07) {
		errMsg := errors.Errorf("Cannot upgrade this version: %v", version)
		return step.ExitUnsafeToReboot{Err: errMsg}
	}

	return nil
}
