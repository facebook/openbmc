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
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(ValidateImagePartitions)
}

func ValidateImagePartitions(stepParams step.StepParams) step.StepExitError {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing image partition validation =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}

	imageData, err := fileutils.MmapFile(stepParams.ImageFilePath,
		syscall.PROT_READ, syscall.MAP_SHARED)
	if err != nil {
		return step.ExitSafeToReboot{
			errors.Errorf("Unable to read image file '%v': %v",
				stepParams.ImageFilePath, err),
		}
	}
	defer fileutils.Munmap(imageData)

	err = validate.ValidateImage(imageData)
	if err != nil {
		return step.ExitUnknownError{err}
	}
	return nil
}
