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
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
)

func init() {
	utils.RegisterStep(validateImageBuildname)
}

func validateImageBuildname(stepParams utils.StepParams) utils.StepExitError {
	err := validate.CheckImageBuildNameCompatibility(stepParams)
	if err != nil {
		return utils.ExitUnknownError{err}
	}
	return nil
}
