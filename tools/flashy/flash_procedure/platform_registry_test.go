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

package flash_procedure

import (
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

func TestPlatformRegistry(t *testing.T) {
	origFlashProcedureMappings := make(map[string]func(step.StepParams) step.StepExitError)
	for k, v := range FlashProcedureMappings {
		origFlashProcedureMappings[k] = v
	}
	defer func() {
		FlashProcedureMappings = origFlashProcedureMappings
	}()

	mockStepParams := step.StepParams{
		ImageFilePath: "/tmp/test_image",
		DeviceID:      "mtd:flash0",
	}

	cases := []struct {
		name           string
		platformName   string
		setupMappings  map[string]func(step.StepParams) step.StepExitError
		wantValid      bool
		wantExecResult step.StepExitError
		expectFuncCall bool
	}{
		{
			name:         "valid platform - wedge100",
			platformName: "wedge100",
			setupMappings: map[string]func(step.StepParams) step.StepExitError{
				"wedge100": func(step.StepParams) step.StepExitError { return nil },
			},
			wantValid:      true,
			wantExecResult: nil,
			expectFuncCall: true,
		},
		{
			name:         "valid platform - fby3",
			platformName: "fby3",
			setupMappings: map[string]func(step.StepParams) step.StepExitError{
				"fby3": func(step.StepParams) step.StepExitError { return step.ExitSafeToReboot{Err: nil} },
			},
			wantValid:      true,
			wantExecResult: step.ExitSafeToReboot{Err: nil},
			expectFuncCall: true,
		},
		{
			name:           "invalid platform - nonexistent",
			platformName:   "nonexistent",
			setupMappings:  map[string]func(step.StepParams) step.StepExitError{},
			wantValid:      false,
			wantExecResult: nil,
			expectFuncCall: false,
		},
		{
			name:           "invalid platform - empty string",
			platformName:   "",
			setupMappings:  map[string]func(step.StepParams) step.StepExitError{},
			wantValid:      false,
			wantExecResult: nil,
			expectFuncCall: false,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			functionCalled := false

			testMappings := make(map[string]func(step.StepParams) step.StepExitError)
			for platform, originalFunc := range tc.setupMappings {
				testMappings[platform] = func(stepParams step.StepParams) step.StepExitError {
					functionCalled = true
					if stepParams.ImageFilePath != mockStepParams.ImageFilePath {
						t.Errorf("ImageFilePath: want %v, got %v", mockStepParams.ImageFilePath, stepParams.ImageFilePath)
					}
					if stepParams.DeviceID != mockStepParams.DeviceID {
						t.Errorf("DeviceID: want %v, got %v", mockStepParams.DeviceID, stepParams.DeviceID)
					}
					return originalFunc(stepParams)
				}
			}
			FlashProcedureMappings = testMappings

			gotValid := PlatformExists(tc.platformName)
			if gotValid != tc.wantValid {
				t.Errorf("PlatformExists(%v): want %v, got %v", tc.platformName, tc.wantValid, gotValid)
			}

			if tc.wantValid {
				gotExecResult := ExecuteFlashProcedure(tc.platformName, mockStepParams)
				if gotExecResult != tc.wantExecResult {
					t.Errorf("ExecuteFlashProcedure result: want %v, got %v", tc.wantExecResult, gotExecResult)
				}

				if tc.expectFuncCall && !functionCalled {
					t.Error("Expected flash function to be called, but it wasn't")
				}
			}
		})
	}
}

func TestPlatformRegistryInitialization(t *testing.T) {
	origFlashProcedureMappings := make(map[string]func(step.StepParams) step.StepExitError)
	for k, v := range FlashProcedureMappings {
		origFlashProcedureMappings[k] = v
	}
	origGeneratedMappings := make(map[string]func(step.StepParams) step.StepExitError)
	for k, v := range GeneratedFlashProcedureMappings {
		origGeneratedMappings[k] = v
	}
	defer func() {
		FlashProcedureMappings = origFlashProcedureMappings
		GeneratedFlashProcedureMappings = origGeneratedMappings
	}()

	cases := []struct {
		name                    string
		generatedMappings       map[string]func(step.StepParams) step.StepExitError
		wantPlatformsInFinal    []string
		wantOverrideApplied     bool
		wantFinalMappingsCount  int
	}{
		{
			name: "basic initialization with overrides",
			generatedMappings: map[string]func(step.StepParams) step.StepExitError{
				"wedge100": func(step.StepParams) step.StepExitError { return nil },
				"fby3":     func(step.StepParams) step.StepExitError { return nil },
				"yosemite": func(step.StepParams) step.StepExitError { return nil },
			},
			wantPlatformsInFinal:   []string{"wedge100", "fby3", "yosemite"},
			wantOverrideApplied:    true,
			wantFinalMappingsCount: 3,
		},
		{
			name: "override only applied if platform exists in generated",
			generatedMappings: map[string]func(step.StepParams) step.StepExitError{
				"wedge100": func(step.StepParams) step.StepExitError { return nil },
			},
			wantPlatformsInFinal:   []string{"wedge100"},
			wantOverrideApplied:    false,
			wantFinalMappingsCount: 1,
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			FlashProcedureMappings = make(map[string]func(step.StepParams) step.StepExitError)
			GeneratedFlashProcedureMappings = tc.generatedMappings

			for platformName, flashFunc := range GeneratedFlashProcedureMappings {
				FlashProcedureMappings[platformName] = flashFunc
			}

			for platformName, overrideFunc := range procedureOverrides {
				if _, exists := GeneratedFlashProcedureMappings[platformName]; exists {
					FlashProcedureMappings[platformName] = overrideFunc
				}
			}

			for _, platform := range tc.wantPlatformsInFinal {
				if _, exists := FlashProcedureMappings[platform]; !exists {
					t.Errorf("Platform '%v' should exist in FlashProcedureMappings", platform)
				}
			}

			if len(FlashProcedureMappings) != tc.wantFinalMappingsCount {
				t.Errorf("FlashProcedureMappings count: want %v, got %v", tc.wantFinalMappingsCount, len(FlashProcedureMappings))
			}

			if tc.wantOverrideApplied {
				if _, exists := FlashProcedureMappings["fby3"]; !exists {
					t.Error("fby3 should exist in FlashProcedureMappings after override")
				}
			}

			for platformName, flashFunc := range FlashProcedureMappings {
				if flashFunc == nil {
					t.Errorf("Flash function for platform '%v' should not be nil", platformName)
				}
			}
		})
	}
}

func TestExecuteFlashProcedureErrors(t *testing.T) {
	mockStepParams := step.StepParams{
		ImageFilePath: "/tmp/test_image",
		DeviceID:      "mtd:flash0",
	}

	cases := []struct {
		name         string
		platformName string
		wantErr      string
	}{
		{
			name:         "unknown platform should return error",
			platformName: "completely_unknown_platform",
			wantErr:      "unknown platform 'completely_unknown_platform' - no flash procedure found",
		},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			if PlatformExists(tc.platformName) {
				t.Errorf("PlatformExists should return false for unknown platform '%v'", tc.platformName)
			}

			result := ExecuteFlashProcedure(tc.platformName, mockStepParams)
			if result == nil {
				t.Errorf("ExecuteFlashProcedure should return an error for unknown platform '%v'", tc.platformName)
				return
			}

			if exitErr, ok := result.(step.ExitSafeToReboot); ok {
				if exitErr.Err.Error() != tc.wantErr {
					t.Errorf("ExecuteFlashProcedure error: want '%v', got '%v'", tc.wantErr, exitErr.Err.Error())
				}
			} else {
				t.Errorf("ExecuteFlashProcedure should return ExitSafeToReboot error, got %T", result)
			}
		})
	}
}
