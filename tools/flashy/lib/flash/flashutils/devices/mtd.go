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

import (
	"fmt"
	"path/filepath"
	"strconv"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type WritableMountedMTD struct {
	Device     string
	Mountpoint string
}

func init() {
	registerFlashDevice("mtd", getMTD)
}

/*
TODO:-
(1) Validate mtd
(2) Check:
	when mtd offset and vboot reinforcement are checked,
	If MTD has a read-only offset, create a new image file with the
	readonly offset from the device and the remaining from the image
	file and use that for flashcp.
*/
func getMTD(deviceSpecifier string) (*FlashDevice, error) {
	mtdMap, err := getMTDMap(deviceSpecifier)
	if err != nil {
		return nil, err
	}

	filePath := filepath.Join("/dev", mtdMap["dev"])
	fileSize, err := strconv.ParseUint(mtdMap["size"], 16, 64)
	if err != nil {
		return nil, errors.Errorf("Found MTD entry for flash device 'mtd:%v' but got error '%v'",
			deviceSpecifier, err)
	}

	var offset uint64 // TODO

	mtd := FlashDevice{
		"mtd",
		deviceSpecifier,
		filePath,
		fileSize,
		offset,
	}

	return &mtd, nil
}

// from mtd device specifier, get a map containing
// [dev, size, erasesize] values for the mtd obtained from
// /proc/mtd
func getMTDMap(deviceSpecifier string) (map[string]string, error) {
	// read from /proc/mtd
	procMTDBuf, err := utils.ReadFile("/proc/mtd")
	if err != nil {
		return nil, errors.Errorf("Unable to read from /proc/mtd: %v", err)
	}
	procMTD := string(procMTDBuf)

	regEx := fmt.Sprintf("(?m)^(?P<dev>mtd[0-9a-f]+): (?P<size>[0-9a-f]+) (?P<erasesize>[0-9a-f]+) \"%v\"$",
		deviceSpecifier)

	mtdMap, err := utils.GetRegexSubexpMap(regEx, procMTD)
	if err != nil {
		return nil, errors.Errorf("Error finding MTD entry in /proc/mtd for flash device 'mtd:%v'",
			deviceSpecifier)
	}
	return mtdMap, nil
}

// return all writable mounted MTDs as specified in /proc/mounts
var GetWritableMountedMTDs = func() ([]WritableMountedMTD, error) {
	writableMountedMTDs := []WritableMountedMTD{}

	procMountsBuf, err := utils.ReadFile("/proc/mounts")
	if err != nil {
		return writableMountedMTDs,
			errors.Errorf("Unable to get writable mounted MTDs: Cannot read /proc/mounts: %v", err)
	}
	procMounts := string(procMountsBuf)

	// device, mountpoint, filesystem, options, dump_freq, fsck_pass
	regEx := `(?m)^(?P<device>/dev/mtd(?:block)?[0-9]+) (?P<mountpoint>[^ ]+) [^ ]+ [^ ]*rw[^ ]* [0-9]+ [0-9]+$`

	allMTDMaps, err := utils.GetAllRegexSubexpMap(regEx, procMounts)
	if err != nil {
		return writableMountedMTDs,
			errors.Errorf("Unable to get writable mounted MTDs: %v", err)
	}

	for _, mtdMap := range allMTDMaps {
		writableMountedMTDs =
			append(writableMountedMTDs,
				WritableMountedMTD{
					mtdMap["device"],
					mtdMap["mountpoint"],
				},
			)
	}

	return writableMountedMTDs, nil
}
