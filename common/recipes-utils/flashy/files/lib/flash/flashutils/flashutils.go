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

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
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

func GetFlashDevice(deviceID string) (*devices.FlashDevice, error) {
	deviceType, deviceSpecifier, err := parseDeviceID(deviceID)
	if err != nil {
		return nil, errors.Errorf("Failed to get flash device: %v", err)
	}

	if getter, ok := devices.FlashDeviceGetterMap[deviceType]; !ok {
		errMsg := fmt.Sprintf("'%v' is not a valid registered flash device type", deviceType)
		return nil, errors.Errorf("Failed to get flash device: %v", errMsg)
	} else {
		return getter(deviceSpecifier)
	}
}
