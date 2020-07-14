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
	"bytes"
	"log"
	"os"
	"testing"

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/lib/utils"
	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/tests"
	"github.com/pkg/errors"
)

func TestEnsureEnoughFreeRam(t *testing.T) {
	// save and defer restore GetMemInfo and getMinMemoryNeeded
	getMemInfoOrig := utils.GetMemInfo
	getMinMemoryNeededOrig := getMinMemoryNeeded

	defer func() {
		utils.GetMemInfo = getMemInfoOrig
		getMinMemoryNeeded = getMinMemoryNeededOrig
	}()

	cases := []struct {
		name               string
		memInfo            *utils.MemInfo
		memInfoErr         error
		minMemoryNeeded    uint64
		minMemoryNeededErr error
		want               utils.StepExitError
	}{
		{
			name: "Enough free ram",
			memInfo: &utils.MemInfo{
				10240000,
				512000,
			},
			memInfoErr:         nil,
			minMemoryNeeded:    511000,
			minMemoryNeededErr: nil,
			want:               nil,
		},
		{
			name: "Not enough free ram",
			memInfo: &utils.MemInfo{
				10240000,
				512000,
			},
			memInfoErr:         nil,
			minMemoryNeeded:    513000,
			minMemoryNeededErr: nil,
			want: utils.ExitSafeToReboot{
				errors.Errorf("Free memory (512000 B) < minimum memory needed (513000 B), reboot needed"),
			},
		},
		{
			name:               "Error in GetMemInfo",
			memInfo:            nil,
			memInfoErr:         errors.Errorf("MemInfo error"),
			minMemoryNeeded:    511000,
			minMemoryNeededErr: nil,
			want:               utils.ExitSafeToReboot{errors.Errorf("MemInfo error")},
		},
		{
			name: "Error in getMinMemoryNeeded",
			memInfo: &utils.MemInfo{
				10240000,
				512000,
			},
			memInfoErr:         nil,
			minMemoryNeeded:    0,
			minMemoryNeededErr: errors.Errorf("MinMemoryNeeded error"),
			want:               utils.ExitSafeToReboot{errors.Errorf("MinMemoryNeeded error")},
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			utils.GetMemInfo = func() (*utils.MemInfo, error) {
				return tc.memInfo, tc.memInfoErr
			}
			getMinMemoryNeeded = func(memInto *utils.MemInfo) (uint64, error) {
				return tc.minMemoryNeeded, tc.minMemoryNeededErr
			}

			got := ensureEnoughFreeRam("x", "x")

			tests.CompareTestExitErrors(tc.want, got, t)
		})
	}

}

func TestGetMinMemoryNeeded(t *testing.T) {
	// save log output into buf for testing
	var buf bytes.Buffer
	log.SetOutput(&buf)
	// save and defer GetHealthdConfig, HealthdExists
	getHealthdConfigOrig := utils.GetHealthdConfig
	healthdExistsOrig := utils.HealthdExists
	defer func() {
		log.SetOutput(os.Stderr)
		utils.GetHealthdConfig = getHealthdConfigOrig
		utils.HealthdExists = healthdExistsOrig
	}()

	cases := []struct {
		name             string
		memInfo          *utils.MemInfo
		healthdExists    bool
		healthdConfig    *utils.HealthdConfig
		healthdConfigErr error
		logContainsSeq   []string
		wantMinMemory    uint64
		wantErr          error
	}{
		{
			name: "No healthd",
			memInfo: &utils.MemInfo{
				1024000,
				512000,
			},
			healthdExists:    false,
			healthdConfig:    nil,
			healthdConfigErr: nil,
			logContainsSeq: []string{
				"Healthd does not exist in this system.",
				"Using default threshold (47185920 B)",
			},
			wantMinMemory: defaultThreshold,
			wantErr:       nil,
		},
		{
			name: "Healthd exists",
			memInfo: &utils.MemInfo{
				1024000,
				512000,
			},
			healthdExists:    true,
			healthdConfig:    &tests.ExampleMinilaketbHealthdConfig,
			healthdConfigErr: nil,
			logContainsSeq: []string{
				"Healthd exists: getting healthd reboot threshold.",
				"Healthd reboot threshold at 95 percent",
				"Using healthd reboot threshold (47237120 B)",
			},
			wantMinMemory: 47237120,
			wantErr:       nil,
		},
		{
			name: "Healthd exists but error getting its threshold",
			memInfo: &utils.MemInfo{
				1024000,
				512000,
			},
			healthdExists:    true,
			healthdConfig:    nil,
			healthdConfigErr: errors.Errorf("Healthd config error"),
			logContainsSeq: []string{
				"Healthd exists: getting healthd reboot threshold.",
			},
			wantMinMemory: 0,
			wantErr:       errors.Errorf("Healthd config error"),
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			buf = bytes.Buffer{}
			utils.GetHealthdConfig = func() (*utils.HealthdConfig, error) {
				return tc.healthdConfig, tc.healthdConfigErr
			}
			utils.HealthdExists = func() bool {
				return tc.healthdExists
			}
			gotMinMemory, err := getMinMemoryNeeded(tc.memInfo)

			tests.CompareTestErrors(tc.wantErr, err, t)
			if gotMinMemory != tc.wantMinMemory {
				t.Errorf("minMemory: '%v' got '%v'", tc.wantMinMemory, gotMinMemory)
			}
			tests.LogContainsSeqTest(buf.String(), tc.logContainsSeq, t)
		})
	}
}
