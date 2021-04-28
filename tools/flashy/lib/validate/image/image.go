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

package image

import (
	"log"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/pkg/errors"
)

// validateImageFileSie ensures that the image file size is smaller than the size of the
// flash device.
// Precondition: deviceID is non-empty and is a valid deviceID.
var validateImageFileSize = func(imageFilePath string, deviceID string) error {
	imageSize, err := fileutils.GetFileSize(imageFilePath)
	if err != nil {
		return errors.Errorf("Unable to get size of image file '%v': %v",
			imageFilePath, err)
	}
	flashDevice, err := flashutils.GetFlashDevice(deviceID)
	if err != nil {
		return errors.Errorf("Unable to get flash device from deviceID '%v': %v",
			deviceID, err)
	}
	flashDeviceSize := flashDevice.GetFileSize()
	if uint64(imageSize) > flashDeviceSize {
		return errors.Errorf("Image size (%vB) larger than flash device size (%vB)",
			imageSize, flashDeviceSize)
	}
	return nil
}

// ValidateImageFile takes in a path to an image file and runs
// Validate over the data.
// Takes in maybeDeviceI denoting a deviceID, which if non-empty is used to
// get the flash device and ensure that the image file size is smaller than the size of the
// flash device.
var ValidateImageFile = func(imageFilePath string, maybeDeviceID string) error {
	if len(maybeDeviceID) == 0 {
		log.Printf("deviceID empty, bypassing image file size check.")
	} else {
		err := validateImageFileSize(imageFilePath, maybeDeviceID)
		if err != nil {
			return errors.Errorf("Image file size check failed: %v", err)
		}
	}
	imageData, err := fileutils.MmapFile(imageFilePath,
		syscall.PROT_READ, syscall.MAP_SHARED)
	if err != nil {
		return errors.Errorf("Unable to read image file '%v': %v",
			imageFilePath, err)
	}
	defer fileutils.Munmap(imageData)

	return validate.Validate(imageData, partition.ImageFormats)
}
