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
	"github.com/pkg/errors"
	"log"
	"regexp"
	"testing"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
)

func TestMmapLenG6(t *testing.T) {
	t.Run("Check if MMAP_LEN makes sense", func(t *testing.T) {
		if FMC_WDT2 + 4 > MMAP_LEN {
			t.Errorf("MMAP_LEN (0x%x) MUST be greater than FMC_WDT2 + 4 (0x%x)", MMAP_LEN, FMC_WDT2 + 4)
		}

		if FMC_WDT2_CLEAR_OFF >= 4 {
			t.Errorf("FMC_WDT2_CLEAR_OFF (0x%x) MUST be less than 4", FMC_WDT2_CLEAR_OFF)
		}
	})
}

func TestExposeRealFlash0OnSecondaryBootAstG6(t *testing.T) {
	FMC_WDT2_ORIG := FMC_WDT2
	MmapDevMemRwOrig := MmapDevMemRw

	defer func() { FMC_WDT2 = FMC_WDT2_ORIG }()
	defer func() { MmapDevMemRw = MmapDevMemRwOrig }()

	mock_mem := []byte{0, 0, 0, 0}

	FMC_WDT2 = 0
	MmapDevMemRw = func() ([]byte, error) { return mock_mem, nil }

	t.Run("Should not call ResetFMC_WDT2() if flag is not active", func(t *testing.T) {
		mock_mem = []byte{0, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetFMC_WDT2Orig := ResetFMC_WDT2
		defer func() { ResetFMC_WDT2 = ResetFMC_WDT2Orig }()
		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetFMC_WDT2 = func(mem []byte) error { return errors.Errorf("Should not have been called") }
		utils.GetMachine = func() (string, error) { return "armv7l", nil }

		res := ExposeRealFlash0OnSecondaryBootG6(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBootG6() to return nil, got %v", res)
		}

		re_expected_log_buffer := "^[^\n]+WDT2 second boot code flag @ 0x0 not active [(]current value = 0x0[)], skipping step\n$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}
	})

	t.Run("Should call ResetFMC_WDT2() if flag is active", func(t *testing.T) {
		mock_mem = []byte{0x10, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetFMC_WDT2Orig := ResetFMC_WDT2
		defer func() { ResetFMC_WDT2 = ResetFMC_WDT2Orig }()
		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetFMC_WDT2 = func(mem []byte) error {
			// Simulate flag clearing on ResetFMC_WDT() call
			mem[0] = 0
			return ResetFMC_WDT2Orig(mem)
		}
		utils.GetMachine = func() (string, error) { return "armv7l", nil }

		res := ExposeRealFlash0OnSecondaryBootG6(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBootG6() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+WDT2 second boot code flag @ 0x0 active [(]current value = 0x10[)], clearing flag...\n" +
			"[^\n]+WDT2 second boot code flag cleared, current value @ 0x0 = 0xea0000\n" +
			"$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

		// Should've written 1 to WDT2_CLR_STATUS_REG_ADDR
		if mock_mem[FMC_WDT2_CLEAR_OFF] != FMC_WDT2_CLEAR_VAL {
			t.Errorf("Expected ResetWDT2StatusRegOrig() to have written to FMC_WDT2")
		}

	})

	t.Run("Should return error if ResetFMC_WDT2() isn't able to reset flag", func(t *testing.T) {
		mock_mem = []byte{0x10, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetFMC_WDT2Orig := ResetFMC_WDT2
		defer func() { ResetFMC_WDT2 = ResetFMC_WDT2Orig }()
		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetFMC_WDT2 = func(mem []byte) error {
			// Simulate flag not clearing on ResetWDT2_FMC() call
			return ResetFMC_WDT2Orig(mem)
		}
		utils.GetMachine = func() (string, error) { return "armv7l", nil }

		res := ExposeRealFlash0OnSecondaryBootG6(step.StepParams{})

		expectedError := step.ExitSafeToReboot{Err: errors.Errorf("Unable to clear WDT2 second boot code flag @ 0x0, current value = 0xea0010")}

		step.CompareTestExitErrors(expectedError, res, t)

		re_expected_log_buffer := "" +
			"^[^\n]+WDT2 second boot code flag @ 0x0 active [(]current value = 0x10[)], clearing flag...\n" +
			"$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})

	t.Run("Should skip if LF OpenBMC", func(t *testing.T) {
		var buf bytes.Buffer
		log.SetOutput(&buf)

		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()
		IsLFOpenBMCOrig := utils.IsLFOpenBMC
		defer func() { utils.IsLFOpenBMC = IsLFOpenBMCOrig }()

		utils.GetMachine = func() (string, error) { return "armv7l", nil }
		utils.IsLFOpenBMC = func() bool { return true }

		res := ExposeRealFlash0OnSecondaryBootG6(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+Skipping step for LF OpenBMC"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})

	t.Run("Should bail out if not running on AST2600", func(t *testing.T) {
		var buf bytes.Buffer
		log.SetOutput(&buf)

		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()
		utils.GetMachine = func() (string, error) { return "armv5tejl", nil }

		res := ExposeRealFlash0OnSecondaryBootG6(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+Remediation handles only AST2600"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})
}
