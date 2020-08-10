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

package flashutils

import (
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// In vboot systems, there exists a 384K region in writable flash (flash1)
// The image needs to be patched with an offset from the bootloader
const vbootOffset = 384 * 1024

// ==== WARNING: THIS STEP ALTERS THE IMAGE FILE ====
// Patch image file with first "offsetBytes" bytes from flash device
var patchImageWithLocalBootloader = func(imageFilePath string, flashDevice devices.FlashDevice, offsetBytes int) error {
	log.Printf("===== WARNING: PATCHING IMAGE FILE =====")
	log.Printf("This vboot system has %vB RO offset in mtd, patching image file "+
		"with offset.", offsetBytes)

	// first check that offsetBytes is within flashDevice file size
	flashDeviceSize := flashDevice.GetFileSize()
	if uint64(offsetBytes) > flashDeviceSize {
		return errors.Errorf("Unable to patch image: offset bytes required (%vB) larger than"+
			"image file size (%vB)", offsetBytes, flashDeviceSize)
	}
	// read all bytes, then get a slice of n bytes to overwrite
	// the image file
	flashDeviceBuf, err := flashDevice.MmapRO()
	if err != nil {
		return errors.Errorf("Unable to patch image: Can't read from flash device: %v",
			err)
	}
	defer flashDevice.Munmap(flashDeviceBuf)

	offsetBuf := flashDeviceBuf[0:offsetBytes]

	err = fileutils.WriteFileWithoutTruncate(imageFilePath, offsetBuf)
	if err != nil {
		return errors.Errorf("Unable to patch image file '%v': %v",
			imageFilePath, err)
	}

	log.Printf("Successfully patched image file")
	return nil
}

var isVbootImagePatchingRequired = func(flashDeviceSpecifier string) (bool, error) {
	vbootEnforcement, err := utils.GetVbootEnforcement()
	if err != nil {
		return false, errors.Errorf("Unable to get vboot enforcement: %v", err)
	}

	// only patch if flash1 and hardware enforce
	// (TODO:- if flash1rw, strip instead)
	if vbootEnforcement == utils.VBOOT_HARDWARE_ENFORCE &&
		flashDeviceSpecifier == "flash1" {
		return true, nil
	}

	return false, nil
}

/*
In hardware-enforced vboot systems (e.g. fbtp), the first 64K of flash1
is Read-Only. There are two ways of getting by this:
(1) patching the first 384K of the image (spl+recovery uboot) from the flash device, then flash flash1
	- done by pypartition (& flashy)
(2) stripping away the first 64K of the image, then flash flash1rw
    - done by fw-util
Flashy goes with (1), as it inherits the tooling from pypartition (which accepts a specification to
flash flash1 instead of flash1rw). Switching to flash1rw suddenly and stripping 64K when flash1
is specified in flashy seems too hacky and will be hard to understand.
Moving to flash1rw is possible when pypartition is completely gone and there is no
tooling/configurations that specifies flashing flash1.
In the future when pypartition is completely gone:
- update tooling and configuration to specify flashing flash1rw
- if flash1 is specified, let it through and just let flashcp fail (fails on fbtp)
- issue: stripping away the first 64K of the image however will cause image validation to fail,
  and is not idempotent (patching the first 384K is), unless the original image (up to 32MB)
  is kept.
*/
// ==== WARNING: THIS STEP CAN ALTER THE IMAGE FILE ====
var VbootPatchImageBootloaderIfNeeded = func(imageFilePath string, flashDevice devices.FlashDevice) error {
	// check if vboot, fail if not a vboot system
	if !utils.IsVbootSystem() {
		return errors.Errorf("Not a vboot system, cannot run vboot remediation")
	}

	// check if image patching is required (required if flash1 has 64K RO header)
	patchRequired, err := isVbootImagePatchingRequired(flashDevice.GetSpecifier())
	if err != nil {
		return errors.Errorf("Unable to determine whether image patching is required: %v", err)
	}

	// if image patching required, patch the image
	if patchRequired {
		err = patchImageWithLocalBootloader(imageFilePath, flashDevice, vbootOffset)
		if err != nil {
			return errors.Errorf("Failed to patch image with local bootloader: %v", err)
		}
	}

	return nil
}
