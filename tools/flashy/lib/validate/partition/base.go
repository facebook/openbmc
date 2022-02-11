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

// Partition is the base interface definition for Partition validation.
type Partition interface {
	GetName() string
	GetSize() uint32
	Validate() error
	GetType() PartitionConfigType
}

// PartitionFactoryArgs contain args for PartitionFactory.
type PartitionFactoryArgs struct {
	Data  []byte
	PInfo PartitionConfigInfo
}

// PartitionFactory is a generic function type to get the corresponding Partition interface.
type PartitionFactory = func(payload PartitionFactoryArgs) Partition

// PartitionFactoryMap maps from PartitionConfigType to the factory function
// that gets the PartitionFactory for that interface.
// Populated by each type of partititon in the corresponding init() function
var PartitionFactoryMap map[PartitionConfigType]PartitionFactory = map[string]PartitionFactory{}

func registerPartitionFactory(partitionConfigType PartitionConfigType, factory PartitionFactory) {
	PartitionFactoryMap[partitionConfigType] = factory
}
