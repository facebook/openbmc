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

// basic tests
func TestFBMetaImagePartition(t *testing.T) {
	exampleData := []byte("abcd")
	args := PartitionFactoryArgs{
		Data: exampleData,
		PInfo: PartitionConfigInfo{
			Name:   "foo",
			Offset: 1024,
			Size:   4,
			Type:   FBMETA_IMAGE,
		},
	}
	wantP := &FBMetaImagePartition{
		Name:   "foo",
		Offset: 1024,
		Data:   exampleData,
	}
	p := fbmetaImagePartitionFactory(args)
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
	if gotValidatorType != FBMETA_IMAGE {
		t.Errorf("validator type: want '%v' got '%v'", FBMETA_IMAGE, gotValidatorType)
	}
}

func TestFBMetaValidate(t *testing.T) {
	parseAndValidateFBImageMetaJSONOrig := parseAndValidateFBImageMetaJSON
	getPartitionConfigsFromFBMetaPartInfosOrig := getPartitionConfigsFromFBMetaPartInfos
	validatePartitionsFromPartitionConfigsOrig := ValidatePartitionsFromPartitionConfigs
	defer func() {
		parseAndValidateFBImageMetaJSON = parseAndValidateFBImageMetaJSONOrig
		getPartitionConfigsFromFBMetaPartInfos = getPartitionConfigsFromFBMetaPartInfosOrig
		ValidatePartitionsFromPartitionConfigs = validatePartitionsFromPartitionConfigsOrig
	}()

	// make metaImageMetaPartitionOffset x bytes representing example data before meta partition
	examplePartitionData := bytes.Repeat([]byte("x"), fbmetaImageMetaPartitionOffset)

	// make metaImageMetaPartitionSize y bytes representing example meta partition data
	exampleMetaPartitionData := bytes.Repeat([]byte("y"), fbmetaImageMetaPartitionSize)

	exampleData := utils.SafeAppendBytes(examplePartitionData, exampleMetaPartitionData)

	exampleMetaInfo := FBMetaInfo{
		FBOBMC_IMAGE_META_VER: 1,
		PartInfos: []FBMetaPartInfo{
			{
				Name:   "foo",
				Offset: 42,
				Size:   4200,
				Type:   FBMETA_RAW,
			},
		},
	}
	examplePartitionConfigs := []PartitionConfigInfo{
		{
			Name:   "foo",
			Offset: 42,
			Size:   4200,
			Type:   FBMETA_MD5,
		},
	}

	cases := []struct {
		name                                      string
		parseAndValidateFBImageMetaJSONErr        error
		getPartitionConfigsFromFBMetaPartInfosErr error
		validatePartitionsFromPartitionConfigsErr error
		want                                      error
	}{
		{
			name:                               "basic passing",
			parseAndValidateFBImageMetaJSONErr: nil,
			getPartitionConfigsFromFBMetaPartInfosErr: nil,
			validatePartitionsFromPartitionConfigsErr: nil,
			want: nil,
		},
		{
			name:                               "parsing error",
			parseAndValidateFBImageMetaJSONErr: errors.Errorf("parsing error"),
			getPartitionConfigsFromFBMetaPartInfosErr: nil,
			validatePartitionsFromPartitionConfigsErr: nil,
			want: errors.Errorf("parsing error"),
		},
		{
			name:                               "getPartitionsConfigs error",
			parseAndValidateFBImageMetaJSONErr: nil,
			getPartitionConfigsFromFBMetaPartInfosErr: errors.Errorf("getPartitionsConfigs error"),
			validatePartitionsFromPartitionConfigsErr: nil,
			want: errors.Errorf("getPartitionsConfigs error"),
		},
		{
			name:                               "validatePartitions error",
			parseAndValidateFBImageMetaJSONErr: nil,
			getPartitionConfigsFromFBMetaPartInfosErr: nil,
			validatePartitionsFromPartitionConfigsErr: errors.Errorf("validatePartitions error"),
			want: errors.Errorf("validatePartitions error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			parseAndValidateFBImageMetaJSON = func(data []byte) (FBMetaInfo, error) {
				if !reflect.DeepEqual(exampleMetaPartitionData, data) {
					t.Errorf("data passed into parseAndValidateImageMetaJSON incorrect")
				}
				return exampleMetaInfo, tc.parseAndValidateFBImageMetaJSONErr
			}
			getPartitionConfigsFromFBMetaPartInfos = func(metaPartInfos []FBMetaPartInfo) ([]PartitionConfigInfo, error) {
				if !reflect.DeepEqual(metaPartInfos, exampleMetaInfo.PartInfos) {
					t.Errorf("part infos: want '%v' got '%v'",
						exampleMetaInfo.PartInfos, metaPartInfos)
				}
				return examplePartitionConfigs, tc.getPartitionConfigsFromFBMetaPartInfosErr
			}
			ValidatePartitionsFromPartitionConfigs = func(
				data []byte,
				partitionConfigs []PartitionConfigInfo,
			) error {
				if !reflect.DeepEqual(exampleData, data) {
					t.Errorf("data passed into ValidatePartitionsFromPartitionConfigs incorrect")
				}
				if !reflect.DeepEqual(examplePartitionConfigs, partitionConfigs) {
					t.Errorf("partition configs: want '%v' got '%v'",
						examplePartitionConfigs, partitionConfigs)
				}
				return tc.validatePartitionsFromPartitionConfigsErr
			}

			p := &FBMetaImagePartition{
				Name:   "foo",
				Data:   exampleData,
				Offset: 0,
			}

			got := p.Validate()

			tests.CompareTestErrors(tc.want, got, t)
		})
	}

}

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

func TestGetPartitionConfigsFromFBMetaPartInfos(t *testing.T) {
	// mock and defer restore GetVbootEnforcement
	getVbootEnforcementOrig := utils.GetVbootEnforcement
	defer func() {
		utils.GetVbootEnforcement = getVbootEnforcementOrig
	}()
	exampleValidMetaPartInfo1 := FBMetaPartInfo{
		Name:     "one",
		Size:     1024,
		Offset:   1024,
		Type:     "rom",
		Checksum: "1",
	}
	exampleValidPartitionConfig1Vboot := PartitionConfigInfo{
		Name:     "one",
		Size:     1024,
		Offset:   1024,
		Type:     IGNORE,
		Checksum: "1",
	}
	exampleValidPartitionConfig1NoVboot := PartitionConfigInfo{
		Name:     "one",
		Size:     1024,
		Offset:   1024,
		Type:     FBMETA_MD5,
		Checksum: "1",
	}
	exampleValidMetaPartInfo2 := FBMetaPartInfo{
		Name:     "two",
		Size:     1024 * 2,
		Offset:   1024 * 2,
		Type:     "fit",
		Checksum: "2",
	}
	exampleValidPartitionConfig2 := PartitionConfigInfo{
		Name:     "two",
		Size:     1024 * 2,
		Offset:   1024 * 2,
		Type:     FIT,
		Checksum: "2",
	}
	cases := []struct {
		name             string
		metaPartInfos    []FBMetaPartInfo
		vbootEnforcement utils.VbootEnforcementType
		vbootErr         error
		want             []PartitionConfigInfo
		wantErr          error
	}{
		{
			name: "basic succeed, no vboot",
			metaPartInfos: []FBMetaPartInfo{
				exampleValidMetaPartInfo1,
				exampleValidMetaPartInfo2,
			},
			vbootEnforcement: utils.VBOOT_NONE,
			vbootErr:         nil,
			want: []PartitionConfigInfo{
				exampleValidPartitionConfig1NoVboot,
				exampleValidPartitionConfig2,
			},
			wantErr: nil,
		},
		{
			name: "basic succeed, vboot hardware-enforce",
			metaPartInfos: []FBMetaPartInfo{
				exampleValidMetaPartInfo1,
				exampleValidMetaPartInfo2,
			},
			vbootEnforcement: utils.VBOOT_HARDWARE_ENFORCE,
			vbootErr:         nil,
			want: []PartitionConfigInfo{
				exampleValidPartitionConfig1Vboot,
				exampleValidPartitionConfig2,
			},
			wantErr: nil,
		},
		{
			name:             "failure getting vboot enforcement",
			metaPartInfos:    []FBMetaPartInfo{},
			vbootEnforcement: utils.VBOOT_NONE,
			vbootErr:         errors.Errorf("unknown vboot error"),
			want:             nil,
			wantErr: errors.Errorf("Unable to get vboot enforcement: %v",
				"unknown vboot error"),
		},
		{
			name: "invalid meta part info type",
			metaPartInfos: []FBMetaPartInfo{
				FBMetaPartInfo{
					Name:     "one",
					Size:     1024,
					Offset:   1024,
					Type:     "the quick brown fox jumps over the lazy dog",
					Checksum: "1",
				},
			},
			vbootEnforcement: utils.VBOOT_NONE,
			vbootErr:         nil,
			want:             nil,
			wantErr: errors.Errorf("Unknown partition type in image-meta: " +
				"'the quick brown fox jumps over the lazy dog'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetVbootEnforcement = func() (utils.VbootEnforcementType, error) {
				return tc.vbootEnforcement, tc.vbootErr
			}
			got, err := getPartitionConfigsFromFBMetaPartInfos(tc.metaPartInfos)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if !reflect.DeepEqual(got, tc.want) {
				t.Errorf("want '%v' got '%v'", tc.want, got)
			}
		})
	}
}

// also covers getPartitionConfigTypeFromFBMetaPartInfoType
func TestGetPartitionConfigFromFBMetaPartInfo(t *testing.T) {
	cases := []struct {
		name             string
		metaPartInfoType string
		vbootEnforcement utils.VbootEnforcementType
		wantType         PartitionConfigType
		wantErr          error
	}{
		{
			name:             "FBMETA_ROM hardware-enforce vboot -> ignore",
			metaPartInfoType: FBMETA_ROM,
			vbootEnforcement: utils.VBOOT_HARDWARE_ENFORCE,
			wantType:         IGNORE,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_ROM -> md5",
			metaPartInfoType: FBMETA_ROM,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         FBMETA_MD5,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_RAW -> md5",
			metaPartInfoType: FBMETA_RAW,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         FBMETA_MD5,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_FIT -> fit",
			metaPartInfoType: FBMETA_FIT,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         FIT,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_MTDONLY -> ignore",
			metaPartInfoType: FBMETA_MTDONLY,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         IGNORE,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_META -> ignore",
			metaPartInfoType: FBMETA_META,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         IGNORE,
			wantErr:          nil,
		},
		{
			name:             "FBMETA_DATA -> ignore",
			metaPartInfoType: FBMETA_DATA,
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         IGNORE,
			wantErr:          nil,
		},
		{
			name:             "unknown partition type",
			metaPartInfoType: "foobar",
			vbootEnforcement: utils.VBOOT_NONE,
			wantType:         IGNORE, // not applicable
			wantErr: errors.Errorf("Unknown partition type in image-meta: %v",
				"'foobar'"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			exampleName := "foo"
			exampleSize := uint32(1234)
			exampleOffset := uint32(5678)
			exampleChecksum := "0863c9d321cefd367f5c419f06ce8330"

			metaPartInfo := FBMetaPartInfo{
				Name:     exampleName,
				Size:     exampleSize,
				Offset:   exampleOffset,
				Checksum: exampleChecksum,
				Type:     tc.metaPartInfoType,
			}

			wantPartitionConfig := PartitionConfigInfo{
				Name:     exampleName,
				Size:     exampleSize,
				Offset:   exampleOffset,
				Checksum: exampleChecksum,
				Type:     tc.wantType,
			}

			got, err := getPartitionConfigFromFBMetaPartInfo(metaPartInfo, tc.vbootEnforcement)
			tests.CompareTestErrors(tc.wantErr, err, t)
			if tc.wantErr == nil {
				if !reflect.DeepEqual(wantPartitionConfig, got) {
					t.Errorf("want '%v' got '%v'", wantPartitionConfig, got)
				}
			}
		})
	}
}
