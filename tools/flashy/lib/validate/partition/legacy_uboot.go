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

// /** legacy uboot header struct from
//   *  https://elixir.bootlin.com/u-boot/v2020.07/source/include/image.h
//   */
// #define IH_NMLEN		32
// #include <stdint.h>
// typedef struct image_header {
// 	uint32_t	Ih_magic;	/* Image Header Magic Number	*/
// 	uint32_t	Ih_hcrc;	/* Image Header CRC Checksum	*/
// 	uint32_t	Ih_time;	/* Image Creation Timestamp	*/
// 	uint32_t	Ih_size;	/* Image Data Size		*/
// 	uint32_t	Ih_load;	/* Data	 Load  Address		*/
// 	uint32_t	Ih_ep;		/* Entry Point Address		*/
// 	uint32_t	Ih_dcrc;	/* Image Data CRC Checksum	*/
// 	uint8_t		Ih_os;		/* Operating System		*/
// 	uint8_t		Ih_arch;	/* CPU architecture		*/
// 	uint8_t		Ih_type;	/* Image Type			*/
// 	uint8_t		Ih_comp;	/* Compression Type		*/
// 	uint8_t		Ih_name[IH_NMLEN];	/* Image Name		*/
// } image_header_t;
import "C"

import (
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"log"

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

type LegacyUbootHeader struct {
	Header C.image_header_t
}

const legacyUbootMagic = 0x27051956
const legacyUbootHeaderSize = 64

/**
 *  Partition class which parses a (legacy) U-Boot header. The data region
 *  begins at the end of the header and ends where the size field in the header
 *  indicates. Used for "kernel" and "rootfs" partitions.
 */
type LegacyUbootPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset uint32
	header C.image_header_t
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

// parse the header and verify it
func (p *LegacyUbootPartition) parseAndCheckHeader() error {
	// make sure partition is large enough
	if len(p.Data) < legacyUbootHeaderSize {
		return errors.Errorf("Partition size (%v) smaller than legacy "+
			"U-Boot header size (%v)",
			len(p.Data), legacyUbootHeaderSize)
	}

	// make a copy since it is required to edit the data
	headerData := make([]byte, legacyUbootHeaderSize)
	copy(headerData, p.Data[:legacyUbootHeaderSize])

	var err error
	p.header, err = decodeLegacyUbootHeader(headerData)
	if err != nil {
		return errors.Errorf("Unable to decode header: %v", err)
	}

	headerChecksum := uint32(p.header.Ih_hcrc)

	// zero out the hcrc
	p.header.Ih_hcrc = C.uint(0)
	// encode the header
	headerDataZeroHCRC, err := encodeLegacyUbootHeader(p.header)
	if err != nil {
		return errors.Errorf("Unable to encode header: %v", err)
	}

	calcHeaderChecksum := crc32.ChecksumIEEE(headerDataZeroHCRC)
	if calcHeaderChecksum != headerChecksum {
		return errors.Errorf("Calculated header checksum 0x%X does not match checksum in header 0x%X",
			calcHeaderChecksum, headerChecksum)
	}

	// restore the hcrc
	p.header.Ih_hcrc = C.uint(headerChecksum)
	return nil
}

// verify that the magic in the header matches legacyUbootMagic
func (p *LegacyUbootPartition) checkMagic() error {
	ih_magic := uint32(p.header.Ih_magic)
	if ih_magic != legacyUbootMagic {
		return errors.Errorf("Partition magic 0x%X does not match legacy U-Boot magic 0x%X",
			ih_magic, legacyUbootMagic)
	}
	return nil
}

// verify that data matches data crc32 in header
func (p *LegacyUbootPartition) checkData() error {
	// check that data + header is within p.Data's size
	ih_size := uint32(p.header.Ih_size)
	if ih_size+legacyUbootHeaderSize > uint32(len(p.Data)) {
		return errors.Errorf("Legacy U-Boot partition incomplete, data part too small.")
	}

	ih_dcrc := uint32(p.header.Ih_dcrc)
	calcDataChecksum := crc32.ChecksumIEEE(
		p.Data[legacyUbootHeaderSize : legacyUbootHeaderSize+ih_size],
	)
	if calcDataChecksum != ih_dcrc {
		return errors.Errorf("Calculated legacy U-Boot data checksum 0x%X does not match checksum in header 0x%X",
			calcDataChecksum, ih_dcrc)
	}

	return nil
}

// encode a C.image_header_t into its byte representation
var encodeLegacyUbootHeader = func(header C.image_header_t) ([]byte, error) {
	var b bytes.Buffer
	err := binary.Write(&b, binary.BigEndian, &header)
	if err != nil {
		return nil, err
	}
	return b.Bytes(), nil
}

// deocde a slice of bytes into a C.image_header_t image header
var decodeLegacyUbootHeader = func(data []byte) (C.image_header_t, error) {
	var header C.image_header_t
	r := bytes.NewReader(data)
	err := binary.Read(r, binary.BigEndian, &header)
	if err != nil {
		return header, errors.Errorf("Can't parse legacy uboot header: %v",
			err)
	}
	return header, nil
}
