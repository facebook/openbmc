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
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(LEGACY_UBOOT, legacyUbootPartitionFactory)
}

var legacyUbootPartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &LegacyUbootPartition{
		Name:   args.PInfo.Name,
		Data:   args.Data,
		Offset: args.PInfo.Offset,
	}
}

// legacyUbootHeader is the legacy u-boot format image_header.
// taken from https://elixir.bootlin.com/u-boot/v2020.07/source/include/image.h#L326
type legacyUbootHeader struct {
	Ih_magic uint32    /* Image Header Magic Number	*/
	Ih_hcrc  uint32    /* Image Header CRC Checksum	*/
	Ih_time  uint32    /* Image Creation Timestamp	*/
	Ih_size  uint32    /* Image Data Size		*/
	Ih_load  uint32    /* Data	 Load  Address		*/
	Ih_ep    uint32    /* Entry Point Address		*/
	Ih_dcrc  uint32    /* Image Data CRC Checksum	*/
	Ih_os    uint8     /* Operating System		*/
	Ih_arch  uint8     /* CPU architecture		*/
	Ih_type  uint8     /* Image Type			*/
	Ih_comp  uint8     /* Compression Type		*/
	Ih_name  [32]uint8 /* Image Name		*/
}

const legacyUbootHeaderSize = 64

const legacyUbootMagic = 0x27051956

// LegacyUbootPartition parses a (legacy) U-Boot header. The data region
// begins at the end of the header and ends where the size field in the header
// indicates. Used for "kernel" and "rootfs" partitions.
type LegacyUbootPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset uint32
	header legacyUbootHeader
}

func (p *LegacyUbootPartition) GetName() string {
	return p.Name
}

func (p *LegacyUbootPartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *LegacyUbootPartition) Validate() error {
	var err error

	// make sure Data larger than header size
	if p.GetSize() < legacyUbootHeaderSize {
		return errors.Errorf("Partition size (%v) smaller than legacy U-Boot header size (%v)",
			p.GetSize(), legacyUbootHeaderSize)
	}
	p.header, err = decodeLegacyUbootHeader(p.Data[:legacyUbootHeaderSize])
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

// checkMagic verifies that the magic in the header matches legacyMagic
func (p *LegacyUbootPartition) checkMagic() error {
	if p.header.Ih_magic != legacyUbootMagic {
		return errors.Errorf("Partition magic 0x%X does not match legacy U-Boot magic 0x%X",
			p.header.Ih_magic, legacyUbootMagic)
	}
	return nil
}

// checkData verifies that data matches data crc32 in header
func (p *LegacyUbootPartition) checkData() error {
	// check that data + header is within p.Data's size
	imageDataEnd, err := utils.AddU32(p.header.Ih_size, legacyUbootHeaderSize)
	if err != nil {
		return err
	}
	if imageDataEnd > uint32(len(p.Data)) {
		return errors.Errorf("Legacy U-Boot partition incomplete, data part too small.")
	}

	calcDataChecksum := crc32.ChecksumIEEE(
		p.Data[legacyUbootHeaderSize:imageDataEnd],
	)
	if calcDataChecksum != p.header.Ih_dcrc {
		return errors.Errorf("Calculated legacy U-Boot data checksum 0x%X does not match checksum in header 0x%X",
			calcDataChecksum, p.header.Ih_dcrc)
	}

	return nil
}

// decodeLegacyUbootHeader takes in the section of bytes containing the header and
// returns the legacyUbootHeader struct containing header info after validating it.
func decodeLegacyUbootHeader(data []byte) (legacyUbootHeader, error) {
	var header legacyUbootHeader
	err := binary.Read(bytes.NewBuffer(data[:]), binary.BigEndian, &header)
	if err != nil {
		return header, errors.Errorf("Unable to decode legacy uboot header data into struct: %v",
			err)
	}

	err = header.validate()
	if err != nil {
		return header, err
	}

	return header, nil
}

func (h *legacyUbootHeader) encodeLegacyUbootHeader() ([]byte, error) {
	var b bytes.Buffer
	err := binary.Write(&b, binary.BigEndian, h)
	if err != nil {
		return nil, err
	}
	return b.Bytes(), nil
}

func (h *legacyUbootHeader) validate() error {
	// make a copy of h
	hCopy := *h

	headerChecksum := hCopy.Ih_hcrc

	// set header checksum to 0
	hCopy.Ih_hcrc = 0

	headerData, err := hCopy.encodeLegacyUbootHeader()
	if err != nil {
		return err
	}

	calcHeaderChecksum := crc32.ChecksumIEEE(headerData)
	if calcHeaderChecksum != headerChecksum {
		return errors.Errorf("Calculated header checksum 0x%X does not match checksum in header 0x%X",
			calcHeaderChecksum, headerChecksum)
	}

	return nil
}
