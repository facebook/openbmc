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
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type WritableMountedMTD struct {
	Device     string
	Mountpoint string
}

type MemoryTechnologyDevice struct {
	Specifier string
	FilePath  string
	FileSize  uint64
}

const MtdType = "mtd"

func init() {
	registerFlashDevice(MtdType, getMTD)
}

func (m MemoryTechnologyDevice) GetType() string {
	return MtdType
}

func (m MemoryTechnologyDevice) GetSpecifier() string {
	return m.Specifier
}

func (m MemoryTechnologyDevice) GetFilePath() string {
	return m.FilePath
}

func (m MemoryTechnologyDevice) GetFileSize() uint64 {
	return m.FileSize
}

// mmaps the whole file, readonly
// required to call Munmap to release the buffer
func (m MemoryTechnologyDevice) MmapRO() ([]byte, error) {
	// use mmap
	mmapFilePath, err := m.getMmapFilePath()
	if err != nil {
		return nil, errors.Errorf("Unable to read '%v': %v",
			m.FilePath, err)
	}
	return utils.MmapFileRange(mmapFilePath, 0, int(m.FileSize), syscall.PROT_READ, syscall.MAP_SHARED)
}

func (m MemoryTechnologyDevice) Munmap(buf []byte) error {
	return utils.Munmap(buf)
}

// /dev/mtd5 cannot be mmap-ed, but /dev/mtdblock5 can
// return the latter instead
func (m MemoryTechnologyDevice) getMmapFilePath() (string, error) {
	regEx := `^(?P<devmtdpath>/dev/mtd)(?P<mtdnum>[0-9]+)$`

	mtdPathMap, err := utils.GetRegexSubexpMap(regEx, m.FilePath)
	if err != nil {
		return "", errors.Errorf("Unable to get block file path for '%v': %v",
			m.FilePath, err)
	}

	return fmt.Sprintf(
		"%vblock%v",
		mtdPathMap["devmtdpath"],
		mtdPathMap["mtdnum"],
	), nil
}

/*
TODO:-
(1) Validate mtd
*/
func getMTD(deviceSpecifier string) (FlashDevice, error) {
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

	return MemoryTechnologyDevice{
		deviceSpecifier,
		filePath,
		fileSize,
	}, nil
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
