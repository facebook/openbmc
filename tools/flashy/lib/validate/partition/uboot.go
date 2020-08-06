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
	"crypto/md5"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(UBOOT, ubootPartitionFactory)
}

var ubootPartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &UBootPartition{
		Name:   args.PInfo.Name,
		Data:   args.Data,
		Offset: args.PInfo.Offset,
	}
}

const ubootMagicSize = 4

var ubootMagics = []uint32{
	0x130000EA, 0xB80000EA, 0xBE0000EA, 0xF0000EA,
}

const ubootChecksumsPartitionSize = 1024 * 4

/*
 * Calculates an md5sum for the whole partition and considers it valid if the
 * computed checksum matches any of the appended checksums
 * Currently used for u-boot partition
 *
 * The checksum is stored as json (appended, 4kB)
 */
type UBootPartition struct {
	Name string
	Data []byte
	// offset in the image file
	Offset uint32
}

func (p *UBootPartition) GetName() string {
	return p.Name
}

func (p *UBootPartition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *UBootPartition) Validate() error {
	// check magic
	err := p.checkMagic()
	if err != nil {
		return err
	}

	checksums, err := p.getAppendedChecksums()
	if err != nil {
		return err
	}

	// calculate the actual checksum without the checksum region
	calcChecksum := p.getChecksum()

	if utils.StringFind(calcChecksum, checksums) == -1 {
		errMsg := fmt.Sprintf("'%v' partition md5sum '%v' unrecognized",
			p.Name, calcChecksum)
		log.Printf("%v", errMsg)
		return errors.Errorf(errMsg)
	}
	log.Printf("'%v' partition md5sum '%v' OK",
		p.Name, calcChecksum)

	log.Printf("U-Boot partition '%v' passed validation", p.Name)
	return nil
}

func (p *UBootPartition) GetType() PartitionConfigType {
	return UBOOT
}

// check magic
func (p *UBootPartition) checkMagic() error {
	// check that p.Data is larger than 4 bytes
	if len(p.Data) < 4 {
		return errors.Errorf("'%v' partition too small (%v) to contain U-Boot magic",
			p.Name, len(p.Data))
	}

	magic := binary.BigEndian.Uint32(p.Data[:4])
	if utils.Uint32Find(magic, ubootMagics) == -1 {
		return errors.Errorf("Magic '0x%X' does not match any U-Boot magic", magic)
	}
	return nil
}

// try to get the appended checksums
func (p *UBootPartition) getAppendedChecksums() ([]string, error) {
	// check that image is large enough to contain appended checksums
	if len(p.Data) < ubootChecksumsPartitionSize {
		return nil, errors.Errorf("'%v' partition too small (%v) to be a U-Boot partition",
			p.Name, len(p.Data))
	}

	appendedChecksums, err := getUBootChecksumsFromJSONData(
		p.Data[len(p.Data)-ubootChecksumsPartitionSize:],
	)
	if err != nil {
		return nil, errors.Errorf("Unable to get appended checksums: %v", err)
	}

	log.Printf("'%v' partition has appended checksum(s) %v",
		p.Name, appendedChecksums)
	return appendedChecksums, nil
}

// calculate md5sum of region excluding the appended checksums region
func (p *UBootPartition) getChecksum() string {
	hash := md5.Sum(p.Data[:len(p.Data)-ubootChecksumsPartitionSize])
	checksum := hex.EncodeToString(hash[:])
	return checksum
}

// given the last 4kB of the partition, try to parse and get the appended checksums
// if parsing failed, an error is returned, and the validation falls back to
// external checksum mode in which the MD5 checksum of the whole partition is calculated and
// matched against supplied checksums
func getUBootChecksumsFromJSONData(checksumsBuf []byte) ([]string, error) {
	// the u-boot checksums_json is a JSON object with md5sums as JSON keys
	// e.g: '{"a5ad8133574ceb63d492c5e7a75feb71": "Signed: Wednesday Jul 17 13:44:40  2017"}'
	var checksumsDat map[string]interface{}

	checksumsBuf = bytes.Trim(checksumsBuf, "\x00")

	err := json.Unmarshal(checksumsBuf, &checksumsDat)
	if err != nil {
		return nil, errors.Errorf("Unable to unmarshal appended JSON bytes.")
	}

	checksums := utils.GetStringKeys(checksumsDat)

	return checksums, nil
}
