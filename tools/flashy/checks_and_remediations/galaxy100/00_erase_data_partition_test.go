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

package remediations_galaxy100

import (
	"reflect"
	"strings"
	"testing"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
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
	// mock and defer restore GetFlashDevice, DirExists and RunCommand
	getFlashDeviceOrig := flashutils.GetFlashDevice
	isDataPartitionMountedOrig := utils.IsDataPartitionMounted
	dirExistsOrig := fileutils.DirExists
	runCommandOrig := utils.RunCommand
	defer func() {
		flashutils.GetFlashDevice = getFlashDeviceOrig
		utils.IsDataPartitionMounted = isDataPartitionMountedOrig
		fileutils.DirExists = dirExistsOrig
		utils.RunCommand = runCommandOrig
	}()

	flashDevice := &mockFlashDevice{}

	cases := []struct {
		name              string
		getFlashDeviceErr error
		dirsExist         []string
		dataPartMounted   bool
		dataPartErr       error
		wantCmds          []string
		cmdErr            error
		want              step.StepExitError
	}{
		{
			name:              "found and erased",
			getFlashDeviceErr: nil,
			dirsExist: []string{
				"/mnt/data/Diag_FAB",
				"/mnt/data/Diag_LC",
			},
			dataPartMounted: false,
			dataPartErr:     nil,
			wantCmds:        []string{"flash_eraseall -j /dev/mock"},
			cmdErr:          nil,
			want:            nil,
		},
		{
			name:              "No 'data0' partition found",
			getFlashDeviceErr: errors.Errorf("not found"),
			dirsExist:         []string{},
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{},
			cmdErr:            nil,
			want:              nil,
		},
		{
			name:              "No Diag paths found",
			getFlashDeviceErr: nil,
			dirsExist:         []string{},
			dataPartMounted:   false,
			dataPartErr:       nil,
			wantCmds:          []string{},
			cmdErr:            nil,
			want:              nil,
		},
		{
			name:              "flash_eraseall failed",
			getFlashDeviceErr: nil,
			dirsExist: []string{
				"/mnt/data/Diag_FAB",
				"/mnt/data/Diag_LC",
			},
			dataPartMounted: false,
			dataPartErr:     nil,
			wantCmds:        []string{"flash_eraseall -j /dev/mock"},
			cmdErr:          errors.Errorf("flash_eraseall failed"),
			want: step.ExitSafeToReboot{
				errors.Errorf("Failed to erase data0 partition: flash_eraseall failed"),
			},
		},
		{
			name:              "error checking /mnt/data mount status",
			getFlashDeviceErr: nil,
			dirsExist: []string{
				"/mnt/data/Diag_FAB",
			},
			dataPartMounted: false,
			dataPartErr:     errors.Errorf("check failed"),
			wantCmds:        []string{},
			cmdErr:          nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Failed to check if /mnt/data is mounted: check failed"),
			},
		},
		{
			name:              "/mnt/data still mounted (RO, possibly)",
			getFlashDeviceErr: nil,
			dirsExist: []string{
				"/mnt/data/Diag_FAB",
			},
			dataPartMounted: true,
			dataPartErr:     nil,
			wantCmds:        []string{},
			cmdErr:          nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("/mnt/data is still mounted, this may mean that the " +
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
			fileutils.DirExists = func(filename string) bool {
				return utils.StringFind(filename, tc.dirsExist) >= 0
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
