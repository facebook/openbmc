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

	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// basic tests
func TestFBMetaMD5Partition(t *testing.T) {
	exampleData := []byte("abcd")
	exampleDataMD5Sum := "e2fc714c4727ee9395f324cd2e7f331f"

	args := PartitionFactoryArgs{
		Data: exampleData,
		PInfo: PartitionConfigInfo{
			Name:     "foo",
			Offset:   1024,
			Size:     4,
			Type:     FBMETA_MD5,
			Checksum: exampleDataMD5Sum,
		},
	}
	wantP := &FBMetaMD5Partition{
		Name:     "foo",
		Offset:   1024,
		Data:     exampleData,
		Checksum: exampleDataMD5Sum,
	}

	p := fbmetaMD5PartitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}
	gotName := p.GetName()
	if "foo" != gotName {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if uint32(4) != gotSize {
		t.Errorf("size: want '%v' got '%v'", 4, gotSize)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != FBMETA_MD5 {
		t.Errorf("validator type: want '%v' got '%v'",
			FBMETA_MD5, gotValidatorType)
	}
}

// Validate tests
func TestFBMetaMD5Validate(t *testing.T) {
	exampleData := []byte("abcd")
	exampleDataMD5Sum := "e2fc714c4727ee9395f324cd2e7f331f"

	cases := []struct {
		name     string
		checksum string
		want     error
	}{
		{
			name:     "validation succeeded",
			checksum: exampleDataMD5Sum,
			want:     nil,
		},
		{
			name:     "validation failed",
			checksum: "UWU",
			want: errors.Errorf("'foo' partition MD5Sum " +
				"(e2fc714c4727ee9395f324cd2e7f331f) does not match checksum required (UWU)"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &FBMetaMD5Partition{
				Name:     "foo",
				Offset:   1024,
				Data:     exampleData,
				Checksum: tc.checksum,
			}
			got := p.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
