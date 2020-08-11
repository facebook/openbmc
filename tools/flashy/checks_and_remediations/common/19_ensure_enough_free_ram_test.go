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
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func TestEnsureEnoughFreeRam(t *testing.T) {
	// save and defer restore GetMemInfo and getMinMemoryNeeded
	getMemInfoOrig := utils.GetMemInfo
	defer func() {
		utils.GetMemInfo = getMemInfoOrig
	}()

	cases := []struct {
		name       string
		memInfo    *utils.MemInfo
		memInfoErr error
		want       step.StepExitError
	}{
		{
			name: "Enough free ram",
			memInfo: &utils.MemInfo{
				120 * 1024 * 1024,
				50 * 1024 * 1024,
			},
			memInfoErr: nil,
			want:       nil,
		},
		{
			name: "Not enough free ram",
			memInfo: &utils.MemInfo{
				120 * 1024 * 1024,
				30 * 1024 * 1024,
			},
			memInfoErr: nil,
			want: step.ExitSafeToReboot{
				errors.Errorf("Free memory (31457280 B) < minimum memory needed (47185920 B), reboot needed"),
			},
		},
		{
			name:       "Error in GetMemInfo",
			memInfo:    nil,
			memInfoErr: errors.Errorf("MemInfo error"),
			want:       step.ExitSafeToReboot{errors.Errorf("MemInfo error")},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetMemInfo = func() (*utils.MemInfo, error) {
				return tc.memInfo, tc.memInfoErr
			}

			got := ensureEnoughFreeRam(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}

}
