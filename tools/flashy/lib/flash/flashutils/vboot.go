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
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type VbootEnforcementType = string

const (
	VBOOT_NONE             VbootEnforcementType = "NONE"
	VBOOT_SOFTWARE_ENFORCE                      = "SOFTWARE_ENFORCE"
	VBOOT_HARDWARE_ENFORCE                      = "HARDWARE_ENFORCE"
)

const vbootUtilPath = "/usr/local/bin/vboot-util"

// In vboot systems, there exists a 384K region in writable flash (flash1)
// The image needs to be patched with an offset from the bootloader
const vbootOffset = 384 * 1024

// Primitive method to check whether the system is a vboot system
// Some firmware versions _may_ have this file but is not vboot,
// but for now it is sufficient to use this check
var isVbootSystem = func() bool {
	return fileutils.FileExists(vbootUtilPath)
}

var getVbootUtilContents = func() (string, error) {
	if !isVbootSystem() {
		return "", errors.Errorf("Not a vboot system")
	}

	// due to a bug in vboot-util if it use cached data for rom version it may
	// results in trailing garbage and fail during bytes decode, we need to nuke
	// the cache first as a mitigation
	// this is best-effort, as these files may have already been deleted
	// or may not exist
	utils.LogAndIgnoreErr(fileutils.RemoveFile("/tmp/cache_store/rom_version"))
	utils.LogAndIgnoreErr(fileutils.RemoveFile("/tmp/cache_store/rom_uboot_version"))

	// vboot-util on Tioga Pass 1 v1.9 (and possibly other versions) is a shell
	// script without a shebang line.
	// Check whether it is an ELF file first, if not, add bash in front
	cmd := []string{vbootUtilPath}
	if !fileutils.IsELFFile(vbootUtilPath) {
		// prepend "bash"
		cmd = append([]string{"bash"}, cmd...)
	}

	_, err, stdout, _ := utils.RunCommand(cmd, 30)
	if err != nil {
		return "", errors.Errorf("Unable to get vboot-util info: %v", err)
	}

	return stdout, nil
}

// get the vboot enforcement type of the system
var GetVbootEnforcement = func() (VbootEnforcementType, error) {
	if !isVbootSystem() {
		return VBOOT_NONE, nil
	}

	// check if "romx" is in procMtdBuf
	procMtdBuf, err := fileutils.ReadFile(utils.ProcMtdFilePath)
	if err != nil {
		return VBOOT_NONE, errors.Errorf("Unable to read '%v': %v",
			utils.ProcMtdFilePath, err)
	}
	if !strings.Contains(string(procMtdBuf), "romx") {
		return VBOOT_NONE, nil
	}

	// check flags in vboot-util
	vbootUtilContents, err := getVbootUtilContents()
	if err != nil {
		return VBOOT_NONE, errors.Errorf("Unable to read vboot-util contents: %v", err)
	}
	if strings.Contains(vbootUtilContents, "Flags hardware_enforce:  0x00") &&
		strings.Contains(vbootUtilContents, "Flags software_enforce:  0x01") {
		return VBOOT_SOFTWARE_ENFORCE, nil
	}
	return VBOOT_HARDWARE_ENFORCE, nil
}

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
	vbootEnforcement, err := GetVbootEnforcement()
	if err != nil {
		return false, errors.Errorf("Unable to get vboot enforcement: %v", err)
	}

	// only patch if flash1 and hardware enforce
	// (TODO:- if flash1rw, strip instead)
	if vbootEnforcement == VBOOT_HARDWARE_ENFORCE &&
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
	if !isVbootSystem() {
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
