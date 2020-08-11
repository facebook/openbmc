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
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// parse an example
func TestParseAndValidateFBImageMetaJSONExample(t *testing.T) {
	wantMetaInfo := FBMetaInfo{
		FBOBMC_IMAGE_META_VER: 1,
		PartInfos: []FBMetaPartInfo{
			{
				Size:     262144,
				Type:     "rom",
				Name:     "spl",
				Checksum: "8a9fad3140022e5a81d4ba62b1a85602",
				Offset:   0,
			},
			{
				Size:     655360,
				Type:     "raw",
				Name:     "rec-u-boot",
				Checksum: "d4d03a42cf64083ffb5e1befcee9529b",
				Offset:   262144,
			},
			{
				Size:     65536,
				Type:     "data",
				Name:     "u-boot-env",
				Checksum: "",
				Offset:   917504,
			},
			{
				Size:     65536,
				Type:     "meta",
				Name:     "image-meta",
				Checksum: "",
				Offset:   983040,
			},
			{
				Size:          655360,
				Type:          "fit",
				Name:          "u-boot-fit",
				Checksum:      "",
				Offset:        1048576,
				FitImageNodes: 1,
			},
			{
				Size:          31850496,
				Type:          "fit",
				Name:          "os-fit",
				Checksum:      "",
				Offset:        1703936,
				FitImageNodes: 3,
			},
			{
				Size:     4194304,
				Type:     "mtdonly",
				Name:     "data0",
				Checksum: "",
				Offset:   33554432,
			},
		},
	}

	imageMetaDat := []byte(tests.ExampleFBImageMetaJSON)
	// append \n
	imageMetaDat = append(imageMetaDat, byte('\n'))

	imageMetaChecksumDat := []byte(tests.ExampleFBImageMetaChecksumJSON)
	// append \n
	imageMetaChecksumDat = append(imageMetaChecksumDat, byte('\n'))

	partitionMetaDat := utils.SafeAppendBytes(imageMetaDat, imageMetaChecksumDat)
	// fill the end with null chars until it reaches metaImageMetaPartitionSize
	partitionMetaDat = append(partitionMetaDat,
		bytes.Repeat([]byte{'\x00'}, fbmetaImageMetaPartitionSize-len(partitionMetaDat))...)

	got, err := parseAndValidateFBImageMetaJSON(partitionMetaDat)
	if err != nil {
		t.Error(err)
	}
	if !reflect.DeepEqual(got, wantMetaInfo) {
		t.Errorf("want '%v' got '%v'", wantMetaInfo, got)
	}
}

// test other edge cases (failures)
func TestParseAndValidateFBImageMetaJSON(t *testing.T) {
	cases := []struct {
		name    string
		data    []byte
		wantErr error
	}{
		{
			name: "cannot find two lines",
			data: []byte("42\n420"),
			wantErr: errors.Errorf("Meta partition incomplete: cannot find two lines of " +
				"JSON"),
		},
		{
			name: "can't get checksums",
			data: []byte("42\nUWU\n\x00"),
			wantErr: errors.Errorf("Unable to unmarshal image-meta checksum JSON: %v",
				"invalid character 'U' looking for beginning of value"),
		},
		{
			name: "checksums don't match",
			data: []byte("42\n{\"meta_md5\":\"42\"}\n\x00"),
			wantErr: errors.Errorf("'image-meta' checksum (%v) does not match checksums supplied (%v)",
				"a1d0c6e83f027327d8461063f4ac58a6", "42"),
		},
		{
			name: "checksum matched, can't get metaInfo",
			data: []byte("UWU\n{\"meta_md5\":\"3f705474ba25e72419d16ea64bcf0723\"}\n\x00"),
			wantErr: errors.Errorf("Unable to unmarshal image-meta JSON: %v",
				"invalid character 'U' looking for beginning of value"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			_, err := parseAndValidateFBImageMetaJSON(tc.data)
			tests.CompareTestErrors(tc.wantErr, err, t)
		})
	}
}
