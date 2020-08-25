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
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"log"

	"github.com/pkg/errors"
)

func init() {
	registerPartitionFactory(FBMETA_MD5, fbmetaMD5PartitionFactory)
}

var fbmetaMD5PartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &FBMetaMD5Partition{
		Name:     args.PInfo.Name,
		Data:     args.Data,
		Offset:   args.PInfo.Offset,
		Checksum: args.PInfo.Checksum,
	}
}

// FBMetaMD5Partition validates a partition by calculating the md5sum
// of the whole partition and matches it against the checksum.
// It is similar to UbootPartition, but is simpler as it does not try to look for
// appended checksums nor use known UBoot checksums.
type FBMetaMD5Partition struct {
	Name     string
	Data     []byte
	Offset   uint32
	Checksum string
}

func (p *FBMetaMD5Partition) GetName() string {
	return p.Name
}

func (p *FBMetaMD5Partition) GetSize() uint32 {
	return uint32(len(p.Data))
}

func (p *FBMetaMD5Partition) Validate() error {
	hash := md5.Sum(p.Data)
	calcChecksum := hex.EncodeToString(hash[:])

	if calcChecksum != p.Checksum {
		errMsg := fmt.Sprintf("'%v' partition MD5Sum (%v) does not match checksum required (%v)",
			p.Name, calcChecksum, p.Checksum)
		log.Printf("%v", errMsg)
		return errors.Errorf(errMsg)
	}

	log.Printf("'%v' partition MD5Sum (%v) OK", p.Name, calcChecksum)
	return nil
}

func (p *FBMetaMD5Partition) GetType() PartitionConfigType {
	return FBMETA_MD5
}
