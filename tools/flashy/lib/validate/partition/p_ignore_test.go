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
	"reflect"
	"testing"
)

func TestIgnorePartition(t *testing.T) {
	args := PartitionFactoryArgs{
		PInfo: PartitionConfigInfo{
			Name:   "foo",
			Offset: 1024,
			Size:   2 * 1024,
			Type:   IGNORE,
		},
	}
	wantP := &IgnorePartition{
		Name:   "foo",
		Offset: 1024,
		Size:   2 * 1024,
	}
	p := ignorePartitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}
	gotName := p.GetName()
	if "foo" != gotName {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if uint32(2048) != gotSize {
		t.Errorf("size: want '%v' got '%v'", 2048, gotSize)
	}
	gotValid := p.Validate()
	if gotValid != nil {
		t.Errorf("validate: want nil got %v", gotValid)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != IGNORE {
		t.Errorf("validator type: want '%v' got '%v'", IGNORE, gotValidatorType)
	}
}
