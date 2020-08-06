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
	"crypto/rand"
	"reflect"
	"strconv"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

type mockPartition struct {
	Size          uint32
	ValidationErr error
}

func (m *mockPartition) GetName() string {
	return "mock-partition"
}

func (m *mockPartition) GetSize() uint32 {
	return 42
}

func (m *mockPartition) Validate() error {
	return m.ValidationErr
}

func (m *mockPartition) GetType() partition.PartitionConfigType {
	return "MOCK"
}

func TestValidatePartitions(t *testing.T) {
	cases := []struct {
		name       string
		partitions []partition.Partition
		want       error
	}{
		{
			name: "validation passed",
			partitions: []partition.Partition{
				&mockPartition{ValidationErr: nil},
			},
			want: nil,
		},
		{
			name: "validation failed",
			partitions: []partition.Partition{
				&mockPartition{ValidationErr: errors.Errorf("mock part validation failed")},
			},
			want: errors.Errorf("Partition 'mock-partition' failed validation: " +
				"mock part validation failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got := validatePartitions(tc.partitions)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestGetAllPartitionsFromPartitionConfigs(t *testing.T) {
	// mock and defer restore PartitionFactoryMap
	partitionFactoryMapOrig := partition.PartitionFactoryMap
	defer func() {
		partition.PartitionFactoryMap = partitionFactoryMapOrig
	}()

	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	var mockConfigTypeFoo partition.PartitionConfigType = "FOO"
	var mockConfigTypeUnknown partition.PartitionConfigType = "UNKNOWN"

	var fooGetter = func(args partition.PartitionFactoryArgs) partition.Partition {
		return &mockPartition{Size: args.PInfo.Size}
	}
	partition.PartitionFactoryMap = map[partition.PartitionConfigType]partition.PartitionFactory{
		mockConfigTypeFoo: fooGetter,
	}

	cases := []struct {
		name             string
		partitionConfigs []partition.PartitionConfigInfo
		want             []partition.Partition
		wantErr          error
	}{
		{
			name: "success",
			partitionConfigs: []partition.PartitionConfigInfo{
				{
					Name:   "foobar",
					Offset: 0,
					Size:   1024,
					Type:   mockConfigTypeFoo,
				},
			},
			want: []partition.Partition{
				&mockPartition{Size: 1024},
			},
			wantErr: nil,
		},
		{
			name: "success: get 2",
			partitionConfigs: []partition.PartitionConfigInfo{
				{
					Name:   "foobar1",
					Offset: 0,
					Size:   1024,
					Type:   mockConfigTypeFoo,
				},
				{
					Name:   "foobar2",
					Offset: 1024,
					Size:   2048,
					Type:   mockConfigTypeFoo,
				},
			},
			want: []partition.Partition{
				&mockPartition{Size: 1 * 1024},
				&mockPartition{Size: 2 * 1024},
			},
			wantErr: nil,
		},
		{
			name: "unknown validator type",
			partitionConfigs: []partition.PartitionConfigInfo{
				{
					Name:   "1",
					Offset: 0,
					Size:   1024,
					Type:   mockConfigTypeUnknown,
				},
			},
			want: nil,
			wantErr: errors.Errorf("Failed to get '%v' partition: "+
				" Unknown partition validator type '%v'",
				"1", mockConfigTypeUnknown),
		},
		{
			name: "size too large, truncated",
			partitionConfigs: []partition.PartitionConfigInfo{
				{
					Name:   "foobar",
					Offset: 0,
					Size:   20 * 1024,
					Type:   mockConfigTypeFoo,
				},
			},
			want: []partition.Partition{
				&mockPartition{Size: 4 * 1024},
			},
			wantErr: nil,
		},
		{
			name: "start offset too large",
			partitionConfigs: []partition.PartitionConfigInfo{
				{
					Name:   "foobar",
					Offset: 20 * 1024,
					Size:   1024,
					Type:   mockConfigTypeFoo,
				},
			},
			want: nil,
			wantErr: errors.Errorf("Wanted start offset (%v) larger than image file size (%v)",
				20*1024, 4*1024),
		},
	}
	// compare two partitions slices
	// this is required as DeepEqual does not work well for a slice of interfaces
	comparePartitionSlices := func(a, b []partition.Partition) bool {
		if len(a) != len(b) {
			return false
		}
		for i := 0; i < len(a); i++ {
			if !reflect.DeepEqual(a[i], b[i]) {
				return false
			}
		}
		return true
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got, err := getAllPartitionsFromPartitionConfigs(
				randData,
				tc.partitionConfigs,
			)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !comparePartitionSlices(tc.want, got) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

func TestValidatePartitionsFromPartitionConfigs(t *testing.T) {
	// mock and defer restore getAllPartitionsFromPartitionConfigs &
	// validatePartitions
	getAllPartitionsFromPartitionConfigsOrig := getAllPartitionsFromPartitionConfigs
	validatePartitionsOrig := validatePartitions
	defer func() {
		getAllPartitionsFromPartitionConfigs = getAllPartitionsFromPartitionConfigsOrig
		validatePartitions = validatePartitionsOrig
	}()

	cases := []struct {
		name                  string
		getAllPartitionsErr   error
		validatePartitionsErr error
		want                  error
	}{
		{
			name:                  "succeeded",
			getAllPartitionsErr:   nil,
			validatePartitionsErr: nil,
			want:                  nil,
		},
		{
			name:                  "getAllPartitions error",
			getAllPartitionsErr:   errors.Errorf("failed to get all partitions"),
			validatePartitionsErr: nil,
			want: errors.Errorf("Unable to get all partitions: " +
				"failed to get all partitions"),
		},
		{
			name:                  "validatePartitions error",
			getAllPartitionsErr:   nil,
			validatePartitionsErr: errors.Errorf("Validation failed"),
			want:                  errors.Errorf("Validation failed: Validation failed"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			getAllPartitionsFromPartitionConfigs = func(
				data []byte,
				partitionConfigs []partition.PartitionConfigInfo,
			) ([]partition.Partition, error) {
				return nil, tc.getAllPartitionsErr
			}
			validatePartitions = func(partitions []partition.Partition) error {
				return tc.validatePartitionsErr
			}

			got := validatePartitionsFromPartitionConfigs(
				[]byte{},
				nil,
			)

			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestValidateImage(t *testing.T) {
	// mock and defer restore ImageFormats, validatePartitionsFromPartitionConfigs
	imageFormatsOrig := partition.ImageFormats
	validatePartitionsFromPartitionConfigsOrig := validatePartitionsFromPartitionConfigs
	defer func() {
		partition.ImageFormats = imageFormatsOrig
		validatePartitionsFromPartitionConfigs = validatePartitionsFromPartitionConfigsOrig
	}()

	// get a mock list of Image formats, the keys are incrementing integers from 1 to
	// imageFormatsElems inclusive
	var getMockImageFormats = func(imageFormatsElems int) map[string]([]partition.PartitionConfigInfo) {
		m := make(map[string]([]partition.PartitionConfigInfo))
		for i := 1; i <= imageFormatsElems; i++ {
			m[strconv.Itoa(i)] = []partition.PartitionConfigInfo{}
		}
		return m
	}

	cases := []struct {
		name string
		// how many elements exists in the mocked ImageFormats
		imageFormatsElems      int
		validatePartitionsErrs []error
		want                   error
	}{
		{
			name:                   "succeed on first try",
			imageFormatsElems:      1,
			validatePartitionsErrs: []error{nil},
			want:                   nil,
		},
		{
			name:                   "succeed on second format",
			imageFormatsElems:      2,
			validatePartitionsErrs: []error{errors.Errorf("'1' failed"), nil},
			want:                   nil,
		},
		{
			name:                   "fail on all formats",
			imageFormatsElems:      2,
			validatePartitionsErrs: []error{errors.Errorf("'1' failed"), errors.Errorf("'2' failed")},
			want:                   errors.Errorf("*** FAILED: Image validation failed ***"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			i := 0
			partition.ImageFormats = getMockImageFormats(tc.imageFormatsElems)
			validatePartitionsFromPartitionConfigs = func(
				data []byte,
				partitionConfigs []partition.PartitionConfigInfo,
			) error {
				idx := i
				i++
				return tc.validatePartitionsErrs[idx]
			}

			got := ValidateImage([]byte{})
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
