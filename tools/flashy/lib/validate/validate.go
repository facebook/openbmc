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

package validate

import (
	"log"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/pkg/errors"
)

// Validate tries to validate partitions according to all configs acquired from
// partition.GetImageFormats.
// If one succeeds, return nil. If none succeeds,
// validation has failed, return the error.
// Supports both data from image file and flash device.
var Validate = func(data []byte) error {
	for _, imageFormat := range partition.GetImageFormats() {
		log.Printf("*** Attempting to validate using image format '%v' ***",
			imageFormat.Name)

		err := partition.ValidatePartitionsFromPartitionConfigs(
			data,
			imageFormat.PartitionConfigs,
		)
		if err != nil {
			log.Printf("Image format '%v' failed validation: %v "+
				"\nTrying the next image format if there are any.",
				imageFormat.Name, err)
			continue
		}

		log.Printf("*** PASSED: Validation with image format '%v' ***",
			imageFormat.Name)
		return nil
	}

	errMsg := "*** FAILED: Validation failed ***"
	log.Printf(errMsg)
	return errors.Errorf(errMsg)
}

// ValidateImageFile takes in a path to an image file and runs
// Validate over the data.
var ValidateImageFile = func(imageFilePath string) error {
	imageData, err := fileutils.MmapFile(imageFilePath,
		syscall.PROT_READ, syscall.MAP_SHARED)
	if err != nil {
		return errors.Errorf("Unable to read image file '%v': %v",
			imageFilePath, err)
	}
	defer fileutils.Munmap(imageData)

	return Validate(imageData)
}
