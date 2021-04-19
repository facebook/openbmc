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
	"bytes"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/partition"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

func TestValidate(t *testing.T) {
	// mock and defer restore ImageFormats, ValidatePartitionsFromPartitionConfigs
	getImageFormatsOrig := partition.GetImageFormats
	validatePartitionsFromPartitionConfigsOrig := partition.ValidatePartitionsFromPartitionConfigs
	defer func() {
		partition.GetImageFormats = getImageFormatsOrig
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
			partition.GetImageFormats = func() []partition.ImageFormat {
				return tc.imageFormats
			}
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

			got := Validate(exampleData)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

func TestValidateImageFile(t *testing.T) {
	// mock and defer restore MmapFile and Validate
	mmapFileOrig := fileutils.MmapFile
	validateOrig := Validate
	defer func() {
		fileutils.MmapFile = mmapFileOrig
		Validate = validateOrig
	}()

	const imageFileName = "/opt/upgrade/image"
	imageData := []byte("abcd")

	cases := []struct {
		name        string
		mmapErr     error
		validateErr error
		want        error
	}{
		{
			name:        "succeeded",
			mmapErr:     nil,
			validateErr: nil,
			want:        nil,
		},
		{
			name:        "mmap error",
			mmapErr:     errors.Errorf("mmap error"),
			validateErr: nil,
			want: errors.Errorf("Unable to read image file '%v': %v",
				"/opt/upgrade/image", "mmap error"),
		},
		{
			name:        "validation error",
			mmapErr:     nil,
			validateErr: errors.Errorf("validation failed"),
			want:        errors.Errorf("validation failed"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			fileutils.MmapFile = func(filename string, prot, flags int) ([]byte, error) {
				if filename != imageFileName {
					t.Errorf("filename: want '%v' got '%v'", imageFileName, filename)
				}
				return imageData, tc.mmapErr
			}

			Validate = func(data []byte) error {
				if bytes.Compare(data, imageData) != 0 {
					t.Errorf("data: want '%v' got '%v'", imageData, data)
				}
				return tc.validateErr
			}

			got := ValidateImageFile(imageFileName)
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
