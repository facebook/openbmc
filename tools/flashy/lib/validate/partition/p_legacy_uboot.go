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

package partition

import (
	"hash/crc32"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(LEGACY_UBOOT, legacyUbootPartitionFactory)
}

// keys for the header
// we only get what we require for validation
const (
	legacyUboot_ih_magic = "magic"
	legacyUboot_ih_hcrc  = "header_crc32"
	legacyUboot_ih_size  = "size"
	legacyUboot_ih_dcrc  = "data_crc32"
)

// offsets of values in the header
var legacyHeaderOffsetMap = map[string]uint32{
	legacyUboot_ih_magic: 0,
	legacyUboot_ih_hcrc:  4,
	legacyUboot_ih_size:  12,
	legacyUboot_ih_dcrc:  24,
}

var legacyUbootPartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &LegacyUbootPartition{
		Name:   args.PInfo.Name,
		Data:   args.Data,
		Offset: args.PInfo.Offset,
	}
}

const legacyUbootMagic = 0x27051956
const legacyUbootHeaderSize = 64

/**
 *  LegacyUbootPartition parses a (legacy) U-Boot header. The data region
 *  begins at the end of the header and ends where the size field in the header
 *  indicates. Used for "kernel" and "rootfs" partitions.
 */
type LegacyUbootPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset uint32
	header map[string]uint32
}

func (p *LegacyUbootPartition) GetName() string {
	return p.Name
}

func (p *LegacyUbootPartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *LegacyUbootPartition) Validate() error {
	err := p.parseAndCheckHeader()
	if err != nil {
		return err
	}

	err = p.checkMagic()
	if err != nil {
		return err
	}

	err = p.checkData()
	if err != nil {
		return err
	}

	log.Printf("Legacy U-Boot partition '%v' passed validation", p.Name)
	return nil
}

func (p *LegacyUbootPartition) GetType() PartitionConfigType {
	return LEGACY_UBOOT
}

// parseAndCheckHeader parses the header and verifies it
func (p *LegacyUbootPartition) parseAndCheckHeader() error {
	// make sure partition is large enough
	if len(p.Data) < legacyUbootHeaderSize {
		return errors.Errorf("Partition size (%v) smaller than legacy U-Boot header size (%v)",
			len(p.Data), legacyUbootHeaderSize)
	}

	// make a copy since it is required to edit the data
	headerData := make([]byte, legacyUbootHeaderSize)
	copy(headerData, p.Data[:legacyUbootHeaderSize])

	var err error
	p.header, err = parseLegacyUbootHeaders(headerData)
	if err != nil {
		return errors.Errorf("Unable to parse legacy headers: %v", err)
	}

	headerChecksum := p.header[legacyUboot_ih_hcrc]

	// set the HEADER_CRC32 to 0 before calculating
	headerData, err = utils.SetWord(headerData, 0, legacyHeaderOffsetMap[legacyUboot_ih_hcrc])
	if err != nil {
		return errors.Errorf("Unable to parse legacy headers: %v", err)
	}

	calcHeaderChecksum := crc32.ChecksumIEEE(headerData)
	if calcHeaderChecksum != headerChecksum {
		return errors.Errorf("Calculated header checksum 0x%X does not match checksum in header 0x%X",
			calcHeaderChecksum, headerChecksum)
	}

	return nil
}

// checkMagic verifies that the magic in the header matches legacyMagic
func (p *LegacyUbootPartition) checkMagic() error {
	if p.header[legacyUboot_ih_magic] != legacyUbootMagic {
		return errors.Errorf("Partition magic 0x%X does not match legacy U-Boot magic 0x%X",
			p.header[legacyUboot_ih_magic], legacyUbootMagic)
	}
	return nil
}

// checkData verifies that data matches data crc32 in header
func (p *LegacyUbootPartition) checkData() error {
	// cheeck that data + header is within p.Data's size
	if p.header[legacyUboot_ih_size]+legacyUbootHeaderSize > uint32(len(p.Data)) {
		return errors.Errorf("Legacy U-Boot partition incomplete, data part too small.")
	}

	calcDataChecksum := crc32.ChecksumIEEE(
		p.Data[legacyUbootHeaderSize : legacyUbootHeaderSize+p.header[legacyUboot_ih_size]],
	)
	if calcDataChecksum != p.header[legacyUboot_ih_dcrc] {
		return errors.Errorf("Calculated legacy U-Boot data checksum 0x%X does not match checksum in header 0x%X",
			calcDataChecksum, p.header[legacyUboot_ih_dcrc])
	}

	return nil
}

// parseLegacyUbootHeaders gets all the headers defined in legacyHeaderOffsetMap
// given the header data.
// Each header is assumed to be a 32-bit value. An error is returned
// if the offset is too large compared to the given data.
var parseLegacyUbootHeaders = func(data []byte) (map[string]uint32, error) {
	var err error
	m := make(map[string]uint32, len(legacyHeaderOffsetMap))
	for key, offset := range legacyHeaderOffsetMap {
		m[key], err = utils.GetWord(data, offset)
		if err != nil {
			return nil,
				errors.Errorf("Unable to get header '%v': %v", key, err)
		}
	}
	return m, nil
}
