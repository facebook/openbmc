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

package remediations_yamp

import (
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils"
	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

type mockFlashDevice struct {
}

func (m *mockFlashDevice) GetType() string         { return "mocktype" }
func (m *mockFlashDevice) GetSpecifier() string    { return "mockspec" }
func (m *mockFlashDevice) GetFilePath() string     { return "/dev/mock" }
func (m *mockFlashDevice) GetFileSize() uint64     { return uint64(1234) }
func (m *mockFlashDevice) MmapRO() ([]byte, error) { return nil, nil }
func (m *mockFlashDevice) Munmap([]byte) error     { return nil }
func (m *mockFlashDevice) Validate() error         { return nil }

func TestEraseDataPartition(t *testing.T) {
	getFlashDeviceOrig := flashutils.GetFlashDevice
	isDataPartitionMountedOrig := utils.IsDataPartitionMounted
	runCommandOrig := utils.RunCommand
	getMTDMapFromSpecifierOrig := utils.GetMTDMapFromSpecifier
	defer func() {
		flashutils.GetFlashDevice = getFlashDeviceOrig
		utils.IsDataPartitionMounted = isDataPartitionMountedOrig
		utils.RunCommand = runCommandOrig
		utils.GetMTDMapFromSpecifier = getMTDMapFromSpecifierOrig
	}()

	flashDevice := &mockFlashDevice{}

	cases := []struct {
		name              string
		getFlashDeviceErr error
		eraseSize         string
		dataPartMounted   bool
		dataPartErr       error
		wantCmds          []string
		cmdErr            error
		want              step.StepExitError
	}{
		{
			name:              "found and erased",
			getFlashDeviceErr: nil,
			eraseSize:         "00010000",
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{"flash_eraseall -j /dev/mock"},
			cmdErr:            nil,
			want:              nil,
		},
		{
			name:              "No 'data0' partition found",
			getFlashDeviceErr: errors.Errorf("not found"),
			eraseSize:         "00001000",
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{},
			cmdErr:            nil,
			want:              nil,
		},
		{
			name:              "No Diag paths found",
			getFlashDeviceErr: nil,
			eraseSize:         "00001000",
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{},
			cmdErr:            nil,
			want:              nil,
		},
		{
			name:              "flash_eraseall failed",
			getFlashDeviceErr: nil,
			eraseSize:         "00010000",
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{"flash_eraseall -j /dev/mock"},
			cmdErr:            errors.Errorf("flash_eraseall failed"),
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Failed to erase data0 partition: flash_eraseall failed"),
			},
		},
		{
			name:              "error checking /mnt/data mount status",
			getFlashDeviceErr: nil,
			eraseSize:         "00010000",
			dataPartMounted:   false,
			dataPartErr:       errors.Errorf("check failed"),
			wantCmds:          []string{},
			cmdErr:            nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("Failed to check if /mnt/data is mounted: check failed"),
			},
		},
		{
			name:              "/mnt/data still mounted (RO, possibly)",
			getFlashDeviceErr: nil,
			eraseSize:         "00010000",
			dataPartMounted:   true,
			dataPartErr:       nil,
			wantCmds:          []string{},
			cmdErr:            nil,
			want: step.ExitSafeToReboot{
				Err: errors.Errorf("/mnt/data is still mounted, this may mean that the " +
					"unmount data partition step fell back to remounting RO. This current step " +
					"needs /mnt/data to be completely unmounted!"),
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			gotCmds := []string{}
			flashutils.GetFlashDevice = func(deviceID string) (devices.FlashDevice, error) {
				if deviceID != "mtd:data0" {
					t.Errorf("deviceID: want '%v' got '%v'", "mtd:data0", deviceID)
				}
				return flashDevice, tc.getFlashDeviceErr
			}
			utils.GetMTDMapFromSpecifier = func(deviceID string) (map[string]string, error) {
				m := make(map[string]string)
				m["erasesize"] = tc.eraseSize
				return m, nil
			}
			utils.RunCommand = func(cmdArr []string, timeout time.Duration) (int, error, string, string) {
				gotCmds = append(gotCmds, strings.Join(cmdArr, " "))
				return 0, tc.cmdErr, "", ""
			}
			utils.IsDataPartitionMounted = func() (bool, error) {
				return tc.dataPartMounted, tc.dataPartErr
			}

			got := eraseDataPartition(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
			if !reflect.DeepEqual(tc.wantCmds, gotCmds) {
				t.Errorf("cmds: want '%v' got '%v'", tc.wantCmds, gotCmds)
			}
		})
	}
}
