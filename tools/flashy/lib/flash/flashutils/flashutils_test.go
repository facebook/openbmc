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

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/tests"
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
	// mock and defer restore FlashDeviceFactoryMap
	flashDeviceFactoryMapOrig := devices.FlashDeviceFactoryMap
	defer func() {
		devices.FlashDeviceFactoryMap = flashDeviceFactoryMapOrig
	}()

	sampleFlashDevice := &devices.MemoryTechnologyDevice{
		"flash0",
		"/dev/mtd5",
		uint64(12345678),
	}

	sampleMtdGetterSuccess := func(specifier string) (devices.FlashDevice, error) {
		return sampleFlashDevice, nil
	}

	sampleMtdGetterError := func(specifier string) (devices.FlashDevice, error) {
		return nil, errors.Errorf("Error getting 'mtd:%v'", specifier)
	}

	cases := []struct {
		name                  string
		deviceID              string
		flashDeviceFactoryMap map[string](devices.FlashDeviceFactory)
		want                  devices.FlashDevice
		wantErr               error
	}{
		{
			name:     "basic passing",
			deviceID: "mtd:flash0",
			flashDeviceFactoryMap: map[string](devices.FlashDeviceFactory){
				"mtd": sampleMtdGetterSuccess,
			},
			want:    sampleFlashDevice,
			wantErr: nil,
		},
		{
			name:     "malformed device ID",
			deviceID: "mtd-flash0",
			flashDeviceFactoryMap: map[string](devices.FlashDeviceFactory){
				"mtd": sampleMtdGetterSuccess,
			},
			want: nil,
			wantErr: errors.Errorf("Failed to get flash device: " +
				"Unable to parse device ID 'mtd-flash0': " +
				"No match for regex '^(?P<type>[a-z]+):(?P<specifier>.+)$' for input 'mtd-flash0'"),
		},
		{
			name:                  "Not present in GetterMap",
			deviceID:              "mtd:flash0",
			flashDeviceFactoryMap: map[string](devices.FlashDeviceFactory){},
			want:                  nil,
			wantErr: errors.Errorf("Failed to get flash device: " +
				"'mtd' is not a valid registered flash device type"),
		},
		{
			name:     "Error in FlashDeviceFactory",
			deviceID: "mtd:flash0",
			flashDeviceFactoryMap: map[string](devices.FlashDeviceFactory){
				"mtd": sampleMtdGetterError,
			},
			want:    nil,
			wantErr: errors.Errorf("Error getting 'mtd:flash0'"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			devices.FlashDeviceFactoryMap = tc.flashDeviceFactoryMap
			got, err := GetFlashDevice(tc.deviceID)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if got == nil {
				if tc.want != nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				}
			} else {
				if tc.want == nil {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				} else if !reflect.DeepEqual(tc.want, got) {
					t.Errorf("want '%v' got '%v'", tc.want, got)
				}
			}

		})
	}
}

type mockValidationFlashDevice struct {
	ValidationErr error
}

func (m *mockValidationFlashDevice) GetType() string         { return "mocktype" }
func (m *mockValidationFlashDevice) GetSpecifier() string    { return "mockspec" }
func (m *mockValidationFlashDevice) GetFilePath() string     { return "/dev/mock" }
func (m *mockValidationFlashDevice) GetFileSize() uint64     { return uint64(1234) }
func (m *mockValidationFlashDevice) MmapRO() ([]byte, error) { return nil, nil }
func (m *mockValidationFlashDevice) Munmap([]byte) error     { return nil }
func (m *mockValidationFlashDevice) Validate() error         { return m.ValidationErr }

// covers CheckFlashDeviceValid as well
func TestCheckAnyFlashDeviceValid(t *testing.T) {
	// mock and defer restore GetFlashDevice, IsPfrSystem
	getFlashDeviceOrig := GetFlashDevice
	isPfrSystemOrig := utils.IsPfrSystem
	defer func() {
		GetFlashDevice = getFlashDeviceOrig
		utils.IsPfrSystem = isPfrSystemOrig
	}()

	type getFlashDeviceReturnType struct {
		FD       devices.FlashDevice
		FDGetErr error
	}

	cases := []struct {
		name      string
		pfrSystem bool
		flash0    getFlashDeviceReturnType
		flash1    getFlashDeviceReturnType
		want      error
	}{
		{
			name:      "flash0 passed validation",
			pfrSystem: false,
			flash0: getFlashDeviceReturnType{
				&mockValidationFlashDevice{nil},
				nil,
			},
			flash1: getFlashDeviceReturnType{
				&mockValidationFlashDevice{nil},
				errors.Errorf("Not used"),
			},
			want: nil,
		},
		{
			name:      "flash0 failed to get but flash1 succeeded",
			pfrSystem: false,
			flash0: getFlashDeviceReturnType{
				&mockValidationFlashDevice{nil},
				errors.Errorf("Can't get"),
			},
			flash1: getFlashDeviceReturnType{
				&mockValidationFlashDevice{nil},
				nil,
			},
			want: nil,
		},
		{
			name:      "flash0 failed validation but flash1 succeeded",
			pfrSystem: false,
			flash0: getFlashDeviceReturnType{
				&mockValidationFlashDevice{errors.Errorf("flash0 failed validation")},
				nil,
			},
			flash1: getFlashDeviceReturnType{
				&mockValidationFlashDevice{nil},
				nil,
			},
			want: nil,
		},
		{
			name:      "both flash devices failed validation",
			pfrSystem: false,
			flash0: getFlashDeviceReturnType{
				&mockValidationFlashDevice{errors.Errorf("flash0 failed validation")},
				nil,
			},
			flash1: getFlashDeviceReturnType{
				&mockValidationFlashDevice{errors.Errorf("flash1 failed validation")},
				nil,
			},
			want: errors.Errorf("UNSAFE TO REBOOT: No flash device is valid"),
		},
		{
			name:      "pfr system, skip validation",
			pfrSystem: true,
			flash0: getFlashDeviceReturnType{
				&mockValidationFlashDevice{errors.Errorf("flash0 failed validation")},
				nil,
			},
			flash1: getFlashDeviceReturnType{
				&mockValidationFlashDevice{errors.Errorf("flash1 failed validation")},
				nil,
			},
			want: nil,
		},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.IsPfrSystem = func() bool {
				return tc.pfrSystem
			}

			GetFlashDevice = func(deviceID string) (devices.FlashDevice, error) {
				if deviceID == "mtd:flash0" {
					return tc.flash0.FD, tc.flash0.FDGetErr
				} else if deviceID == "mtd:flash1" {
					return tc.flash1.FD, tc.flash1.FDGetErr
				}
				t.Errorf("Unknown deviceID: %v", deviceID)
				return nil, nil
			}

			got := CheckAnyFlashDeviceValid()
			tests.CompareTestErrors(tc.want, got, t)
		})
	}
}
