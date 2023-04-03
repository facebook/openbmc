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
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// ValidatePartitionsFromPartitionConfigs gets all the partitions based on the partitionConfigs
// then validate them. If all passed, return nil. Else return the error.
var ValidatePartitionsFromPartitionConfigs = func(
	data []byte,
	partitionConfigs []PartitionConfigInfo,
) error {
	partitions, err := getAllPartitionsFromPartitionConfigs(
		data,
		partitionConfigs,
	)
	if err != nil {
		return errors.Errorf("Unable to get all partitions: %v",
			err)
	}
	err = validatePartitions(partitions)
	if err != nil {
		return errors.Errorf("Validation failed: %v", err)
	}
	return nil
}

// validatePartitions calls validate for all partitions, and returns an error if a partition
// failed validation. If all partitions pass validation, return nil.
var validatePartitions = func(partitions []Partition) error {
	for _, p := range partitions {
		log.Printf("Validating partition '%v' using '%v' partition validator",
			p.GetName(), p.GetType())
		err := p.Validate()
		if err != nil {
			return errors.Errorf("Partition '%v' failed validation: %v",
				p.GetName(), err)
		}
	}
	return nil
}

// getAllPartitionsFromPartitionConfigs tries to get all partitions based on partition configs.
// Return an error if unknown partition config type or failed to get.
var getAllPartitionsFromPartitionConfigs = func(
	data []byte,
	partitionConfigs []PartitionConfigInfo,
) ([]Partition, error) {
	partitions := []Partition{}

	for _, pInfo := range partitionConfigs {
		// make sure the beginning offset is not larger than the image
		// partitions of the IGNORE format can be not present in the image, and
		// that is ok.
		if pInfo.Offset > uint32(len(data)) && pInfo.Type != IGNORE {
			return nil, errors.Errorf("Wanted start offset (%v) larger than image file size (%v)",
				pInfo.Offset, len(data))
		}

		// flash size is 32MB, so pInfo may indicate a size
		// larger than the image file. (this is fine with flash devices are they are 32MB)
		// In such a case, reduce the size of the partition in pInfo.
		// The alternative would be to pad the image with zero bytes until 32MB,
		// but that would require operating on a new copy of the image in memory.
		// Golang mmap unfortunately doesn't expose the addr argument to allow
		// fixed mapping (we could map a 23MB image file over a 32MB /dev/zero file)
		var pData []byte
		if uint32(len(data)) > pInfo.Offset {
			pOffsetEnd, err := utils.AddU32(pInfo.Size, pInfo.Offset)
			if err != nil {
				return nil, errors.Errorf("Unable to get offset end: %v", err)
			}

			if uint32(len(data)) < pOffsetEnd {
				actualPartitionSize := uint32(len(data)) - pInfo.Offset
				pInfo.Size = actualPartitionSize
				pOffsetEnd = uint32(len(data))
			}

			// only pass in region of data defined as the partition
			pData, err = utils.BytesSliceRange(data, pInfo.Offset, pOffsetEnd)
			if err != nil {
				return nil, errors.Errorf("Unable to extract partition data: %v", err)
			}
		}
		args := PartitionFactoryArgs{
			Data:  pData,
			PInfo: pInfo,
		}

		if factory, ok := PartitionFactoryMap[pInfo.Type]; ok {
			p := factory(args)
			partitions = append(partitions, p)
		} else {
			return nil, errors.Errorf("Failed to get '%v' partition: "+
				" Unknown partition validator type '%v'",
				pInfo.Name, pInfo.Type)
		}
	}
	return partitions, nil
}
