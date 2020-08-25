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

import "log"

func init() {
	registerPartitionFactory(IGNORE, ignorePartitionFactory)
}

var ignorePartitionFactory = func(args PartitionFactoryArgs) Partition {
	return &IgnorePartition{
		Name:   args.PInfo.Name,
		Offset: args.PInfo.Offset,
		Size:   args.PInfo.Size,
	}
}

// IgnorePartition represents a partition whose validation is ignored.
type IgnorePartition struct {
	Name string
	// offset in the image file
	Offset uint32
	Size   uint32
}

func (p *IgnorePartition) GetName() string {
	return p.Name
}

func (p *IgnorePartition) GetSize() uint32 {
	return p.Size
}

func (p *IgnorePartition) Validate() error {
	log.Printf("Ignore partition '%v' validation ignored", p.Name)
	return nil
}

func (p *IgnorePartition) GetType() PartitionConfigType {
	return IGNORE
}
