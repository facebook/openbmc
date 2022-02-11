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

package utilities

import (
	"fmt"
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
)

func init() {
	step.RegisterUtility(checkImage)
}

func checkImageUsage() {
	fmt.Printf(
		`check-image
-----------
Validates partitions in an image.

Usage: check-image FILE DEVICE_ID

DEVICE_ID is optional and denotes the target flash device ID (e.g. mtd:flash0), which is used
to ensure that the image file size is smaller than that of the target flash device.
`)
}

func checkImage(args []string) error {
	if len(args) < 2 || len(args) > 3 || len(args) > 1 && (strings.Contains(args[1], "help") || args[1] == "-h") {
		checkImageUsage()
		return nil
	}

	imageFilePath := args[1]
	maybeDeviceID := ""
	if len(args) == 3 {
		maybeDeviceID = args[2]
	}
	return image.ValidateImageFile(imageFilePath, maybeDeviceID)
}
