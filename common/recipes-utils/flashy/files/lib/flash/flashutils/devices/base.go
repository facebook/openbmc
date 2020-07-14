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

package devices

type FlashDevice struct {
	Type      string
	Specifier string
	FilePath  string
	FileSize  uint64
	Offset    uint64
}

// takes in the specifier and returns the FlashDevice
type FlashDeviceGetter = func(string) (*FlashDevice, error)

// maps from the type of the device to the getter function
// that gets & validates the information of the flash storage device
// populated by each storage device in ./devices/ via the RegisterFlashDevice
// function
var FlashDeviceGetterMap map[string]FlashDeviceGetter = map[string]FlashDeviceGetter{}

func registerFlashDevice(deviceType string, getter FlashDeviceGetter) {
	FlashDeviceGetterMap[deviceType] = getter
}
