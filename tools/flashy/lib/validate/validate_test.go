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

package validate

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestValidate(t *testing.T) {
	// mock and defer restore ValidatePartitionsFromPartitionConfigs
	validatePartitionsFromPartitionConfigsOrig := partition.ValidatePartitionsFromPartitionConfigs
	defer func() {
		partition.ValidatePartitionsFromPartitionConfigs = validatePartitionsFromPartitionConfigsOrig
	}()

	cases := []struct {
		name                   string
		imageFormats           []partition.ImageFormat
		validatePartitionsErrs []error
		want                   error
	}{
		{
			name: "succeed on first try",
			imageFormats: []partition.ImageFormat{
				{
					Name:             "1",
					PartitionConfigs: []partition.PartitionConfigInfo{},
				},
			},
			validatePartitionsErrs: []error{nil},
			want:                   nil,
		},
		{
			name: "succeed on second format",
			imageFormats: []partition.ImageFormat{
				{
					Name:             "1",
					PartitionConfigs: []partition.PartitionConfigInfo{},
				},
				{
					Name:             "2",
					PartitionConfigs: []partition.PartitionConfigInfo{},
				},
			},
			validatePartitionsErrs: []error{errors.Errorf("'1' failed"), nil},
			want:                   nil,
		},
		{
			name: "fail on all formats",
			imageFormats: []partition.ImageFormat{
				{
					Name:             "1",
					PartitionConfigs: []partition.PartitionConfigInfo{},
				},
				{
					Name:             "2",
					PartitionConfigs: []partition.PartitionConfigInfo{},
				},
			},
			validatePartitionsErrs: []error{errors.Errorf("'1' failed"), errors.Errorf("'2' failed")},
			want:                   errors.Errorf("*** FAILED: Validation failed ***"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleData := []byte("abcd")
			i := 0
			partition.ValidatePartitionsFromPartitionConfigs = func(
				data []byte,
				partitionConfigs []partition.PartitionConfigInfo,
			) error {
				if !reflect.DeepEqual(exampleData, data) {
					t.Errorf("data: want '%v' got '%v'",
						exampleData, data)
				}
				idx := i
				i++
				return tc.validatePartitionsErrs[idx]
			}

			got := Validate(exampleData, tc.imageFormats)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
