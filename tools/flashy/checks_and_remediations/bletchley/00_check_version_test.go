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

package remediations_bletchley

import (
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func TestCheckVersion(t *testing.T) {
	// mock and defer restore RunCommand
	getOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile
	defer func() {
		utils.GetOpenBMCVersionFromIssueFile = getOpenBMCVersionFromIssueFileOrig
	}()
	cases := []struct {
		name      string
		version   string
		err       error
		want      step.StepExitError
	}{
		{
			name:      "failed",
			version:   "bletchley-v2023.02.1",
			err:       nil,
			want: step.ExitUnsafeToReboot{
				errors.Errorf("Cannot upgrade this version: bletchley-v2023.02.1"),
			},
		},
		{
			name:      "succeeded",
			version:   "bletchley-v2023.07.1",
			err:       nil,
			want:      nil,
		},
		{
			name:      "garbage",
			version:   "garbage",
			err:       nil,
			want: step.ExitUnsafeToReboot{
				errors.Errorf("Unable to parse version info: No match for regex 'bletchley-v(?P<year>[0-9]+).(?P<month>[0-9]+)' for input 'garbage'"),
			},
		},
		{
			name:      "error",
			version:   "",
			err:       errors.Errorf("fail"),
			want: step.ExitUnsafeToReboot{
				errors.Errorf("Unable to fetch version info: fail"),
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetOpenBMCVersionFromIssueFile = func() (string, error) {
				return tc.version, tc.err
			}
			got := checkVersion(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
