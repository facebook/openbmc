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

package flashutils

import (
	"reflect"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestParseDeviceID(t *testing.T) {
	cases := []struct {
		name          string
		deviceID      string
		wantType      string
		wantSpecifier string
		wantErr       error
	}{
		{
			name:          "mtd:flash0 -> type=mtd specifier=flash0",
			deviceID:      "mtd:flash0",
			wantType:      "mtd",
			wantSpecifier: "flash0",
			wantErr:       nil,
		},
		{
			name:          "typo: 'mtd-flash0'",
			deviceID:      "mtd-flash0",
			wantType:      "",
			wantSpecifier: "",
			wantErr: errors.Errorf("Unable to parse device ID 'mtd-flash0': " +
				"No match for regex '^(?P<type>[a-z]+):(?P<specifier>.+)$' for input 'mtd-flash0'"),
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			gotType, gotSpecifier, err := parseDeviceID(tc.deviceID)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if tc.wantType != gotType {
				t.Errorf("type: want '%v' got '%v'", tc.wantType, gotType)
			}
			if tc.wantSpecifier != gotSpecifier {
				t.Errorf("specifier: want '%v' got '%v'", tc.wantSpecifier, gotSpecifier)
			}
		})
	}
}

func TestGetFlashDevice(t *testing.T) {
	// mock and defer restore FlashDeviceGetterMap
	flashDeviceGetterMapOrig := devices.FlashDeviceGetterMap
	defer func() {
		devices.FlashDeviceGetterMap = flashDeviceGetterMapOrig
	}()

	sampleFlashDevice := devices.FlashDevice{
		"mtd",
		"flash0",
		"/dev/mtd5",
		uint64(12345678),
		uint64(0),
	}

	sampleMtdGetterSuccess := func(specifier string) (*devices.FlashDevice, error) {
		return &sampleFlashDevice, nil
	}

	sampleMtdGetterError := func(specifier string) (*devices.FlashDevice, error) {
		return nil, errors.Errorf("Error getting 'mtd:%v'", specifier)
	}

	cases := []struct{
		name string
		deviceID string
		flashDeviceGetterMap map[string](devices.FlashDeviceGetter)
		want *devices.FlashDevice
		wantErr error
	}{
		{
			name: "basic passing",
			deviceID: "mtd:flash0",
			flashDeviceGetterMap: map[string](devices.FlashDeviceGetter) {
				"mtd": sampleMtdGetterSuccess,
			},
			want: &sampleFlashDevice,
			wantErr: nil,
		},
		{
			name: "malformed device ID",
			deviceID: "mtd-flash0",
			flashDeviceGetterMap: map[string](devices.FlashDeviceGetter) {
				"mtd": sampleMtdGetterSuccess,
			},
			want: nil,
			wantErr: errors.Errorf("Failed to get flash device: " +
				"Unable to parse device ID 'mtd-flash0': " +
				"No match for regex '^(?P<type>[a-z]+):(?P<specifier>.+)$' for input 'mtd-flash0'"),
		},
		{
			name: "Not present in GetterMap",
			deviceID: "mtd:flash0",
			flashDeviceGetterMap: map[string](devices.FlashDeviceGetter) {},
			want: nil,
			wantErr: errors.Errorf("Failed to get flash device: " +
				"'mtd' is not a valid registered flash device type"),
		},
		{
			name: "Error in FlashDeviceGetter",
			deviceID: "mtd:flash0",
			flashDeviceGetterMap: map[string](devices.FlashDeviceGetter) {
				"mtd": sampleMtdGetterError,
			},
			want: nil,
			wantErr: errors.Errorf("Error getting 'mtd:flash0'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func (t *testing.T) {
			devices.FlashDeviceGetterMap = tc.flashDeviceGetterMap
			got, err := GetFlashDevice(tc.deviceID)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if got == nil {
				if tc.want != nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				}
			} else {
				if tc.want == nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				} else if !reflect.DeepEqual(*tc.want, *got) {
					t.Errorf("want '%v' got '%v'", *tc.want, *got)
				}
			}

		})
	}
}
