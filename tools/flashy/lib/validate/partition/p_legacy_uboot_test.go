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
	"crypto/rand"
	"hash/crc32"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// test basic functionality
func TestGetLegacyUbootPartition(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	args := PartitionFactoryArgs{
		Data: randData,
		PInfo: PartitionConfigInfo{
			Name:   "foo",
			Offset: 2048,
			Size:   1024,
			Type:   LEGACY_UBOOT,
		},
	}

	wantP := &LegacyUbootPartition{
		Name:   "foo",
		Data:   randData,
		Offset: 1024 * 2,
	}

	p := legacyUbootPartitionFactory(args)
	if !reflect.DeepEqual(wantP, p) {
		t.Errorf("partition: want '%v' got '%v'", wantP, p)
	}

	gotName := p.GetName()
	if gotName != "foo" {
		t.Errorf("name: want '%v' got '%v'", "foo", gotName)
	}
	gotSize := p.GetSize()
	if gotSize != 4096 {
		t.Errorf("size: want '%v' got '%v'", 4096, gotSize)
	}
	gotValidatorType := p.GetType()
	if gotValidatorType != LEGACY_UBOOT {
		t.Errorf("validator type: want '%v' got '%v'", LEGACY_UBOOT, gotValidatorType)
	}
}

func TestLegacyUBootValidate(t *testing.T) {
	mockSize := 1024
	mockData := bytes.Repeat([]byte{'x'}, mockSize)
	mockBufCRC32 := crc32.ChecksumIEEE(mockData)
	mockHeader := []byte{
		0x27, 0x05, 0x19, 0x56, // 0: magic
		0x06, 0x32, 0x0B, 0xB7, // 4: header CRC32
		0x00, 0x00, 0x00, 0x00, // 8
		0x00, 0x00, 0x04, 0x00, // 12: size
		0x00, 0x00, 0x00, 0x00, // 16
		0x00, 0x00, 0x00, 0x00, // 20
		byte((mockBufCRC32 >> 24) & 0xFF),
		byte((mockBufCRC32 >> 16) & 0xFF),
		byte((mockBufCRC32 >> 8) & 0xFF),
		byte(mockBufCRC32 & 0xFF), // 24: data CRC32
		0x00, 0x00, 0x00, 0x00,    // 28
		0x00, 0x00, 0x00, 0x00, // 32
		0x00, 0x00, 0x00, 0x00, // 36
		0x00, 0x00, 0x00, 0x00, // 40
		0x00, 0x00, 0x00, 0x00, // 44
		0x00, 0x00, 0x00, 0x00, // 48
		0x00, 0x00, 0x00, 0x00, // 52
		0x00, 0x00, 0x00, 0x00, // 56
		0x00, 0x00, 0x00, 0x00, // 60
	}

	mockHeaderCorruptedMagic := make([]byte, 64)
	copy(mockHeaderCorruptedMagic, mockHeader)
	mockHeaderCorruptedMagic, _ = utils.SetWord(mockHeaderCorruptedMagic, 0, 0)
	mockHeaderCorruptedMagic, _ = utils.SetWord(mockHeaderCorruptedMagic, 0x40167B94, 4)

	mockHeaderCorruptedHeaderCRC32 := make([]byte, 64)
	copy(mockHeaderCorruptedHeaderCRC32, mockHeader)
	mockHeaderCorruptedHeaderCRC32, _ = utils.SetWord(mockHeaderCorruptedHeaderCRC32, 0, 4)

	mockHeaderCorruptedDataCRC32 := make([]byte, 64)
	copy(mockHeaderCorruptedDataCRC32, mockHeader)
	mockHeaderCorruptedDataCRC32, _ = utils.SetWord(mockHeaderCorruptedDataCRC32, 0, 24)
	mockHeaderCorruptedDataCRC32, _ = utils.SetWord(mockHeaderCorruptedDataCRC32, 0x4EFE7E12, 4)

	cases := []struct {
		name string
		data []byte
		want error
	}{
		{
			name: "basic successful",
			data: utils.SafeAppendBytes(mockHeader, mockData),
			want: nil,
		},
		{
			name: "too short for header",
			data: []byte("foo"),
			want: errors.Errorf("Partition size (%v) smaller than legacy U-Boot header size (%v)",
				3, legacyUbootHeaderSize),
		},
		{
			name: "header checksum does not match",
			data: utils.SafeAppendBytes(mockHeaderCorruptedHeaderCRC32, mockData),
			want: errors.Errorf("Calculated header checksum 0x%X does not match checksum in header 0x%X",
				0x06320BB7, 0),
		},
		{
			name: "magic does not match",
			data: utils.SafeAppendBytes(mockHeaderCorruptedMagic, mockData),
			want: errors.Errorf("Partition magic 0x%X does not match legacy U-Boot magic 0x%X",
				0, legacyUbootMagic),
		},
		{
			name: "data part smaller than specified in header",
			data: utils.SafeAppendBytes(mockHeader, []byte("x")),
			want: errors.Errorf("Legacy U-Boot partition incomplete, data part too small."),
		},
		{
			name: "data checksum does not match",
			data: utils.SafeAppendBytes(mockHeaderCorruptedDataCRC32, mockData),
			want: errors.Errorf("Calculated legacy U-Boot data checksum 0x%X does not match checksum in header 0x%X",
				mockBufCRC32, 0),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			p := &LegacyUbootPartition{
				Name: "foo",
				Data: tc.data,
			}

			got := p.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

// tests validate, encodeLegacyUbootHeader and decodeLegacyUbootHeader
func TestLegacyUbootHeader(t *testing.T) {
	mockHeader := []byte{
		0x27, 0x05, 0x19, 0x56, // 0: magic
		0x36, 0x37, 0x8E, 0x24, // 4: header CRC32
		0x00, 0x00, 0x00, 0x00, // 8
		0x00, 0x00, 0x04, 0x00, // 12: size
		0x00, 0x00, 0x00, 0x00, // 16
		0x00, 0x00, 0x00, 0x00, // 20
		0x12, 0x34, 0x56, 0x78, // 24: data CRC32
		0x00, 0x00, 0x00, 0x00, // 28
		0x00, 0x00, 0x00, 0x00, // 32
		0x00, 0x00, 0x00, 0x00, // 36
		0x00, 0x00, 0x00, 0x00, // 40
		0x00, 0x00, 0x00, 0x00, // 44
		0x00, 0x00, 0x00, 0x00, // 48
		0x00, 0x00, 0x00, 0x00, // 52
		0x00, 0x00, 0x00, 0x00, // 56
		0x00, 0x00, 0x00, 0x00, // 60
	}

	got, err := decodeLegacyUbootHeader(mockHeader)
	if err != nil {
		t.Error(err)
	}
	gotIh_magic := uint32(got.Ih_magic)
	wantIh_magic := uint32(0x27051956)
	if wantIh_magic != gotIh_magic {
		t.Errorf("Ih_magic: want 0x%x got 0x%x", wantIh_magic, gotIh_magic)
	}
	gotIh_hcrc := uint32(got.Ih_hcrc)
	wantIh_hcrc := uint32(0x36378e24)
	if wantIh_hcrc != gotIh_hcrc {
		t.Errorf("Ih_hcrc: want 0x%x got 0x%x", wantIh_hcrc, gotIh_hcrc)
	}
	gotIh_size := uint32(got.Ih_size)
	wantIh_size := uint32(0x00000400)
	if wantIh_size != gotIh_size {
		t.Errorf("Ih_size: want 0x%x got 0x%x", wantIh_size, gotIh_size)
	}
	gotIh_dcrc := uint32(got.Ih_dcrc)
	wantIh_drcrc := uint32(0x12345678)
	if wantIh_drcrc != gotIh_dcrc {
		t.Errorf("Ih_dcrc: want 0x%x got 0x%x", wantIh_drcrc, gotIh_dcrc)
	}

	// encode it and check equality
	encoded, err := got.encodeLegacyUbootHeader()
	if err != nil {
		t.Error(err)
	}
	if bytes.Compare(encoded, mockHeader) != 0 {
		t.Errorf("encode decode failed: want '%v' got '%v'", mockHeader, encoded)
	}

	// make it an error by giving it data that is too small
	_, err = decodeLegacyUbootHeader([]byte("foo"))
	tests.CompareTestErrors(errors.Errorf("Unable to decode legacy uboot header data into struct: unexpected EOF"),
		err, t)

}
