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

package remediations_wedge100

import (
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(fixROMCS1)
}

const gpioUtilPath = "/usr/local/bin/openbmc_gpio_util.py"

// For wedge100, there was a bug that caused the CS1 pin to be configured
// for GPIO instead. Manually configure it for Chip Select before attempting
// to write to flash1.
// N.B.: This should be fixed by D16911862, but there is no guarantee that
// all wedge100s are upgraded.
func fixROMCS1(stepParams step.StepParams) step.StepExitError {
	_, err, _, _ := utils.RunCommand([]string{gpioUtilPath, "config", "ROMCS1#"},
		60*time.Second)
	if err != nil {
		errMsg := errors.Errorf("Failed to run ROMCS1# fix: %v", err)
		return step.ExitSafeToReboot{Err: errMsg}
	}
	return nil
}
