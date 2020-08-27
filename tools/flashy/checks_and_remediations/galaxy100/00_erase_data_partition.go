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

package remediations_galaxy100

import (
	"log"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(eraseDataPartition)
}

var diagPaths = []string{
	"/mnt/data/Diag_FAB",
	"/mnt/data/Diag_LC",
}

// eraseDataPartition works around JFFS failures caused by Diag_LC and Diag_FAB
// in the /mnt/data partition.
// Galaxy has LCs (line cards) and FABs (fabric cards).
// This step can be removed after next galaxy card update.
func eraseDataPartition(stepParams step.StepParams) step.StepExitError {
	data0, err := flashutils.GetFlashDevice("mtd:data0")
	if err != nil {
		log.Printf("Skipping this step: no data0 partition found: %v", err)
		return nil
	}

	for _, p := range diagPaths {
		if fileutils.DirExists(p) {
			log.Printf("Galaxy100 card with '%v' present, erasing data0 partition '%v'.",
				p, data0)

			eraseCmd := []string{
				"flash_eraseall",
				"-j",
				data0.GetFilePath(),
			}
			_, err, _, _ := utils.RunCommand(eraseCmd, 15*time.Minute)
			if err != nil {
				return step.ExitSafeToReboot{
					errors.Errorf("Failed to erase data0 partition: %v", err),
				}
			}
			log.Printf("Successfully erased data0 partition.")
			return nil
		}
	}

	return nil
}
