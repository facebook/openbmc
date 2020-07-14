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
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/flash/flash_procedures"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
)

func init() {
	utils.RegisterStepEntryPoint(flashWedge100)
}

var flashWedge100 = flash_procedures.FlashCp
