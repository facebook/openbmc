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
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// parseDeviceID parses a given device ID and returns (type, specifier, nil).
// It returns an error if parsing failed.
// e.g. “mtd:flash0” -> type=”mtd” specifier=”flash0”
// TODO:- e.g. “emmc:0” -> type=”emmc” specifier=”0”
// TODO:- e.g. “pfr:primary” -> type=”pfr” specifier=”primary”
// TODO:- e.g. “file:/dev/sda” -> type=”file” specifier=”/dev/sda”
var parseDeviceID = func(deviceID string) (string, string, error) {
	const rType = "type"
	const rSpecifier = "specifier"

	regEx := fmt.Sprintf(`^(?P<%v>[a-z]+):(?P<%v>.+)$`, rType, rSpecifier)

	flashDeviceMap, err := utils.GetRegexSubexpMap(regEx, deviceID)
	if err != nil {
		return "", "", errors.Errorf("Unable to parse device ID '%v': %v",
			deviceID, err)
	}

	return flashDeviceMap[rType], flashDeviceMap[rSpecifier], nil
}

// GetFlashDevice returns the FlashDevice given the device ID.
var GetFlashDevice = func(deviceID string) (devices.FlashDevice, error) {
	deviceType, deviceSpecifier, err := parseDeviceID(deviceID)
	if err != nil {
		return nil, errors.Errorf("Failed to get flash device: %v", err)
	}

	if factory, ok := devices.FlashDeviceFactoryMap[deviceType]; !ok {
		errMsg := fmt.Sprintf("'%v' is not a valid registered flash device type", deviceType)
		return nil, errors.Errorf("Failed to get flash device: %v", errMsg)
	} else {
		return factory(deviceSpecifier)
	}
}

// CheckFlashDeviceValid validates the device specified by deviceID.
// An error is returned if the device failed validation.
var CheckFlashDeviceValid = func(deviceID string) error {
	flashDevice, err := GetFlashDevice(deviceID)
	if err != nil {
		return errors.Errorf("Unable to get flash device '%v': %v",
			deviceID, err)
	}
	err = flashDevice.Validate()
	if err != nil {
		return errors.Errorf("Flash device '%v' failed validation: %v",
			deviceID, err)
	}
	// passed
	return nil
}

// CheckAnyFlashDeviceValid returns nil if any of the two
// flash devices (mtd:flash0, mtd:flash1) are valid.
// This is used to check whether it is actually safe to reboot;
// if both flash devices are corrupt, the device will brick upon reboot.
// If the device only exposes one flash, the other flash is deemed invalid.
// For PFR devices, this step is skipped, since it is guaranteed to be safe to
// reboot in this context.
var CheckAnyFlashDeviceValid = func() error {
	if utils.IsPfrSystem() {
		log.Printf("Skipping flash device validation: this is a PFR system")
		return nil
	}

	flashDeviceIDs := []string{
		"mtd:flash0",
		"mtd:flash1",
	}
	// LF-OpenBMC uses "bmc" and "alt-bmc".
	if utils.IsLFOpenBMC() {
		flashDeviceIDs = []string{
			"mtd:bmc",
			"mtd:alt-bmc",
		}
	}

	for _, deviceID := range flashDeviceIDs {
		err := CheckFlashDeviceValid(deviceID)
		if err != nil {
			log.Printf("Flash device '%v' failed validation: %v",
				deviceID, err)
			continue
		} else {
			// passed
			log.Printf("Flash device '%v' passed validation.", deviceID)
			return nil
		}
	}

	return errors.Errorf("UNSAFE TO REBOOT: No flash device is valid")
}
