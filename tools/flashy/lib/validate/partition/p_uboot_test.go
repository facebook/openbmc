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
	"crypto/md5"
	"crypto/rand"
	"encoding/hex"
	"fmt"
	"reflect"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
	"github.com/pkg/errors"
)

// test basic functionality
func TestUBootPartition(t *testing.T) {
	// make 4kB random bytes
	randData := make([]byte, 4*1024)
	rand.Read(randData)

	args := PartitionFactoryArgs{
		Data: randData,
		PInfo: PartitionConfigInfo{
			Name:   "foo",
			Offset: 2048,
			Size:   4096,
			Type:   UBOOT,
		},
	}

	wantP := &UBootPartition{
		Name:      "foo",
		Data:      randData,
		Offset:    2048,
		checksums: ubootKnownChecksums,
	}

	p := ubootPartitionFactory(args)
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
	if gotValidatorType != UBOOT {
		t.Errorf("validator type: want '%v' got '%v'", UBOOT, gotValidatorType)
	}
}

// tests checkMagic, getAppendedChecksums, getChecksum
func TestUBootValidate(t *testing.T) {
	// mock and defer restore ubootKnownChecksums
	ubootKnownChecksumsOrig := ubootKnownChecksums
	defer func() {
		ubootKnownChecksums = ubootKnownChecksumsOrig
	}()

	exampleMagic := []byte{0x13, 0x00, 0x00, 0xEA}

	// 4kB of x
	mockBuf := bytes.Repeat([]byte("x"), 4*1024)

	mockPartitionData := utils.SafeAppendBytes(exampleMagic, mockBuf)
	hash_mockPartitionData := md5.Sum(mockPartitionData)
	md5_mockPartitionData := hex.EncodeToString(hash_mockPartitionData[:])

	md5string_mockPartitionData :=
		fmt.Sprintf("{ \"%v\": 123 }", md5_mockPartitionData)

	// pad with null chars until 4k
	md5bytes_mockPartitionData :=
		utils.SafeAppendBytes(
			[]byte(md5string_mockPartitionData),
			bytes.Repeat([]byte("\x00"), ubootChecksumsPartitionSize-len(md5string_mockPartitionData)),
		)

	// append the checksums
	mockFullPartition :=
		utils.SafeAppendBytes(mockPartitionData, md5bytes_mockPartitionData)

	cases := []struct {
		name string
		// replaces ubootKnownChecksums
		checksums []string
		data      []byte
		want      error
	}{
		{
			name:      "checksums appended, ok",
			checksums: []string{},
			data:      mockFullPartition,
			want:      nil,
		},
		{
			name:      "checksums appended, not OK",
			checksums: []string{},
			// lose some bytes in the middle
			data: utils.SafeAppendBytes(
				mockFullPartition[:128],
				mockFullPartition[256:],
			),
			want: errors.Errorf(
				fmt.Sprintf("'u-boot' partition md5sum '%v' unrecognized",
					"79bdf03034a1524e674125e98bb07530"),
			),
		},
		{
			name:      "checksums not appended, md5sum not OK",
			checksums: []string{},
			data:      mockPartitionData,
			want: errors.Errorf("'u-boot' partition md5sum '%v' unrecognized",
				md5_mockPartitionData),
		},
		{
			name:      "checksums not appended, md5sum matches with known checksums",
			checksums: []string{md5_mockPartitionData},
			data:      mockPartitionData,
			want:      nil,
		},
		{
			name:      "partition too small to have magic",
			checksums: []string{},
			data:      []byte("1"),
			want:      errors.Errorf("'u-boot' partition too small (1) to contain U-Boot magic"),
		},
		{
			name:      "partition too small to have appended checksums",
			checksums: []string{"5b4b599718c1e0bb8fc67d671557fdb1"},
			data:      exampleMagic,
			want:      nil,
		},
		{
			name:      "magic not in uboot magics",
			checksums: []string{},
			// lose the first byte
			data: mockFullPartition[1:],
			want: errors.Errorf("Magic '0x%X' does not match any U-Boot magic", 0xEA78),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			ubootKnownChecksums = tc.checksums
			ubootPart := ubootPartitionFactory(
				PartitionFactoryArgs{
					Data: tc.data,
					PInfo: PartitionConfigInfo{
						Name:   "u-boot",
						Offset: 0,
						Size:   uint32(len(tc.data)),
						Type:   UBOOT,
					},
				},
			)
			got := ubootPart.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}

// validation tests related to VBoot Enforcement
func TestUBootValidateVboot(t *testing.T) {
	// mock and defer restore GetVbootEnforcement
	getVbootEnforcementOrig := utils.GetVbootEnforcement
	defer func() {
		utils.GetVbootEnforcement = getVbootEnforcementOrig
	}()

	cases := []struct {
		name     string
		vbootEnf utils.VbootEnforcementType
		want     error
	}{
		{
			name:     "hardware enforce, bypass uboot check",
			vbootEnf: utils.VBOOT_HARDWARE_ENFORCE,
			want:     nil,
		},
		{
			name:     "not hardware enforce",
			vbootEnf: utils.VBOOT_NONE,
			want:     errors.Errorf("'foobar' partition too small (0) to contain U-Boot magic"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetVbootEnforcement = func() utils.VbootEnforcementType {
				return tc.vbootEnf
			}
			p := &UBootPartition{
				Name: "foobar",
			}

			got := p.Validate()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
