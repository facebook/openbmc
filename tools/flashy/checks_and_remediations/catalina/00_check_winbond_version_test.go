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

package remediations_catalina

import (
	"fmt"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func TestCheckWinbondVersion(t *testing.T) {
	// mock and defer functions
	getBSMFlashManufacturerFromFileOrig := utils.GetBSMFlashManufacturerFromFile
	defer func() {
		utils.GetBSMFlashManufacturerFromFile = getBSMFlashManufacturerFromFileOrig
	}()
	getOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile
	defer func() {
		utils.GetOpenBMCVersionFromIssueFile = getOpenBMCVersionFromIssueFileOrig
	}()
	cases := []struct {
		name         string
		manufacturer string
		m_err        error
		version      string
		v_err        error
		want         step.StepExitError
	}{
		{
			name:         "Winbond - S483584 affected 1",
			manufacturer: "winbond",
			m_err:        nil,
			version:      "catalina-v2024.46.1-QuantaRelease",
			v_err:        nil,
			want: step.ExitUnsafeToReboot{
				fmt.Errorf("Cannot upgrade this version (catalina-v2024.46.1-QuantaRelease) due to S483584"),
			},
		},
		{
			name:         "Winbond - S483584 affected 2",
			manufacturer: "winbond",
			m_err:        nil,
			version:      "catalina-v2025.03.3",
			v_err:        nil,
			want: step.ExitUnsafeToReboot{
				fmt.Errorf("Cannot upgrade this version (catalina-v2025.03.3) due to S483584"),
			},
		},
		{
			name:         "Winbond - not S483584 affected",
			manufacturer: "winbond",
			m_err:        nil,
			version:      "catalina-v2025.03.4",
			v_err:        nil,
			want:         nil,
		},
		{
			name:         "Winbond - bitbake version",
			manufacturer: "winbond",
			m_err:        nil,
			version:      "catalina-a5s3na1",
			v_err:        nil,
			want:         nil,
		},
		{
			name:         "Not Winbond",
			manufacturer: "mellanox",
			m_err:        nil,
			version:      "catalina-v2025.02.1",
			v_err:        nil,
			want:         nil,
		},
		{
			name:         "Manufacturer read file err",
			manufacturer: "derp",
			m_err:        fmt.Errorf("fail"),
			version:      "catalina-v2025.02.1",
			v_err:        nil,
			want: step.ExitUnsafeToReboot{
				fmt.Errorf("Unable to fetch SPI Manufacturer info: fail"),
			},
		},
		{
			name:         "Version read file err",
			manufacturer: "winbond",
			m_err:        nil,
			version:      "",
			v_err:        fmt.Errorf("fail"),
			want: step.ExitUnsafeToReboot{
				fmt.Errorf("Unable to fetch version info: fail"),
			},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetBSMFlashManufacturerFromFile = func() (string, error) {
				return tc.manufacturer, tc.m_err
			}
			utils.GetOpenBMCVersionFromIssueFile = func() (string, error) {
				return tc.version, tc.v_err
			}
			got := checkWinbondVersion(step.StepParams{})
			step.CompareTestExitErrors(tc.want, got, t)
		})
	}
}
