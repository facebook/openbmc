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

	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/pkg/errors"
)

// Validate tries to validate partitions according to all configs defined in
// imageFormats. If one succeeds, return nil. If none succeeds,
// validation has failed, return the error.
// Supports both data from image file and flash device
var Validate = func(data []byte, imageFormats []partition.ImageFormat) error {
	for _, imageFormat := range imageFormats {
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
	log.Print(errMsg)
	return errors.Errorf(errMsg)
}
