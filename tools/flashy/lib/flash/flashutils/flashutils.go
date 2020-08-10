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

/*
 e.g. “mtd:flash0” -> type=”mtd” specifier=”flash0”
 TODO:- e.g. “emmc:0” -> type=”emmc” specifier=”0”
 TODO:- e.g. “pfr:primary” -> type=”pfr” specifier=”primary”
 TODO:- e.g. “file:/dev/sda” -> type=”file” specifier=”/dev/sda”
*/
// parses device ID and returns (type, specifier, nil)
// returns an error if parsing failed
var parseDeviceID = func(deviceID string) (string, string, error) {
	regEx := `^(?P<type>[a-z]+):(?P<specifier>.+)$`

	flashDeviceMap, err := utils.GetRegexSubexpMap(regEx, deviceID)
	if err != nil {
		return "", "", errors.Errorf("Unable to parse device ID '%v': %v",
			deviceID, err)
	}

	return flashDeviceMap["type"], flashDeviceMap["specifier"], nil
}

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

// return nil if any of the two flash devices (mtd:flash0, mtd:flash1)
// are valid. Used to check whether it is actually safe to reboot;
// if both flash devices are corrtupt, the device will brick upon reboot.
// If the device only exposes one flash, the other flash is deemed invalid.
var CheckAnyFlashDeviceValid = func() error {
	flashDeviceIDs := []string{"mtd:flash0", "mtd:flash1"}

	for _, f := range flashDeviceIDs {
		flashDevice, err := GetFlashDevice(f)
		if err != nil {
			log.Printf("Flash device '%v' failed validation: %v",
				f, err)
			continue
		}
		err = flashDevice.Validate()
		if err != nil {
			log.Printf("Flash device '%v' failed validation: %v",
				f, err)
		} else {
			// passed
			log.Printf("Flash device '%v' passed validation.", f)
			return nil
		}
	}

	return errors.Errorf("UNSAFE TO REBOOT: No flash device is valid")
}
