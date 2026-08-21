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
package common

import (
	"fmt"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func TestEnsureEnoughFreeRAM(t *testing.T) {
	// save and defer restore GetMemInfo and GetOpenBMCPlatformFromIssueFile
	getMemInfoOrig := utils.GetMemInfo
	getOpenBMCPlatformFromIssueFileOrig := utils.GetOpenBMCPlatformFromIssueFile
	defer func() {
		utils.GetMemInfo = getMemInfoOrig
		utils.GetOpenBMCPlatformFromIssueFile = getOpenBMCPlatformFromIssueFileOrig
	}()

	cases := []struct {
		name        string
		memInfo     *utils.MemInfo
		memInfoErr  error
		platform    string
		platformErr error
		want        step.StepExitError
	}{
		{
			name: "Enough free ram",
			memInfo: &utils.MemInfo{
				MemTotal: 120 * 1024 * 1024,
				MemFree:  50 * 1024 * 1024,
			},
			memInfoErr: nil,
			platform:   "fbtp",
			want:       nil,
		},
		{
			name: "Not enough free ram",
			memInfo: &utils.MemInfo{
				MemTotal: 120 * 1024 * 1024,
				MemFree:  30 * 1024 * 1024,
			},
			memInfoErr: nil,
			platform:   "fbtp",
			want: step.ExitSafeToReboot{
				Err: fmt.Errorf("Free memory (31457280 B) < minimum memory needed (47185920 B), reboot needed"),
			},
		},
		{
			// 35M is below the 45M default but above wedge100's relaxed 30M limit
			name: "wedge100 allows the relaxed limit",
			memInfo: &utils.MemInfo{
				MemTotal: 240 * 1024 * 1024,
				MemFree:  35 * 1024 * 1024,
			},
			memInfoErr: nil,
			platform:   "wedge100",
			want:       nil,
		},
		{
			name: "wedge100 still enforces the relaxed limit",
			memInfo: &utils.MemInfo{
				MemTotal: 240 * 1024 * 1024,
				MemFree:  25 * 1024 * 1024,
			},
			memInfoErr: nil,
			platform:   "wedge100",
			want: step.ExitSafeToReboot{
				Err: fmt.Errorf("Free memory (26214400 B) < minimum memory needed (31457280 B), reboot needed"),
			},
		},
		{
			// Same free memory as the wedge100 case above: the relaxation must not
			// leak to any other platform
			name: "Relaxed limit does not apply to wedge400",
			memInfo: &utils.MemInfo{
				MemTotal: 240 * 1024 * 1024,
				MemFree:  35 * 1024 * 1024,
			},
			memInfoErr: nil,
			platform:   "wedge400",
			want: step.ExitSafeToReboot{
				Err: fmt.Errorf("Free memory (36700160 B) < minimum memory needed (47185920 B), reboot needed"),
			},
		},
		{
			// /etc/issue is always expected to be readable, so bail rather than guess
			name: "Error getting platform",
			memInfo: &utils.MemInfo{
				MemTotal: 240 * 1024 * 1024,
				MemFree:  35 * 1024 * 1024,
			},
			memInfoErr:  nil,
			platformErr: fmt.Errorf("Error reading /etc/issue"),
			want: step.ExitSafeToReboot{
				Err: fmt.Errorf("Unable to determine platform: Error reading /etc/issue"),
			},
		},
		{
			name:       "Error in GetMemInfo",
			memInfo:    nil,
			memInfoErr: fmt.Errorf("MemInfo error"),
			platform:   "fbtp",
			want:       step.ExitSafeToReboot{Err: fmt.Errorf("MemInfo error")},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetMemInfo = func() (*utils.MemInfo, error) {
				return tc.memInfo, tc.memInfoErr
			}
			utils.GetOpenBMCPlatformFromIssueFile = func() (string, error) {
				return tc.platform, tc.platformErr
			}

			got := ensureEnoughFreeRAM(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}

}
