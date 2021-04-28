/**
 * Copyright 2021-present Facebook. All Rights Reserved.
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

package remediations_grandcanyon

import (
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
)

func init() {
	step.RegisterStep(validateImageMetaPartitions)
}

// validateMetaPartitions validates the image file against the meta partition format.
// This is for platforms that are to be retrofitted to vboot/fit and will block
// flashes that go back to now unsupported formats.
// Note that checks_and_remediations/common/41_validate_image_partitions will already have
// validated the image, but this step is more strict. It is worth the extra check here.
func validateImageMetaPartitions(stepParams step.StepParams) step.StepExitError {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing image partition validation =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}

	// wrap into a slice
	imageFormats := []partition.ImageFormat{partition.MetaPartitionImageFormat}

	err := image.ValidateImageFile(
		stepParams.ImageFilePath,
		stepParams.DeviceID,
		imageFormats,
	)
	if err != nil {
		// the image could've been corrupted during copy,
		// it is safe to reboot.
		return step.ExitSafeToReboot{Err: err}
	}
	return nil
}
