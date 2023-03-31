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

func TestMmapLen(t *testing.T) {
	t.Run("Check if MMAP_LEN makes sense", func(t *testing.T) {
		if WDT2_STATUS_REG_ADDR > MMAP_LEN {
			t.Errorf("MMAP_LEN (0x%x) MUST be greater than WDT2_STATUS_REG_ADDR (0x%x)", MMAP_LEN, WDT2_STATUS_REG_ADDR)
		}

		if WDT2_CLR_STATUS_REG_ADDR > MMAP_LEN {
			t.Errorf("MMAP_LEN (0x%x) MUST be greater than WDT2_CLR_STATUS_REG_ADDR (0x%x)", MMAP_LEN, WDT2_CLR_STATUS_REG_ADDR)
		}
	})
}

func TestExposeRealFlash0OnSecondaryBoot(t *testing.T) {
	WDT2_STATUS_REG_ADDR_ORIG := WDT2_STATUS_REG_ADDR
	WDT2_CLR_STATUS_REG_ADDR_ORIG := WDT2_CLR_STATUS_REG_ADDR
	MmapDevMemRwOrig := MmapDevMemRw
	GetOpenBMCVersionFromIssueFileOrig := utils.GetOpenBMCVersionFromIssueFile


	defer func() { WDT2_STATUS_REG_ADDR = WDT2_STATUS_REG_ADDR_ORIG }()
	defer func() { WDT2_CLR_STATUS_REG_ADDR = WDT2_CLR_STATUS_REG_ADDR_ORIG }()
	defer func() { MmapDevMemRw = MmapDevMemRwOrig }()
	defer func() { utils.GetOpenBMCVersionFromIssueFile = GetOpenBMCVersionFromIssueFileOrig }()

	mock_mem := []byte{0, 1, 0, 0, 0, 0, 0, 0}

	WDT2_STATUS_REG_ADDR = 0
	WDT2_CLR_STATUS_REG_ADDR = 4
	MmapDevMemRw = func() ([]byte, error) { return mock_mem, nil }
	utils.GetOpenBMCVersionFromIssueFile = func() (string, error) { return "wedge400-v2023.03.0", nil }

	t.Run("Should not call ResetWDT2StatusReg() if flag is not active", func(t *testing.T) {
		mock_mem = []byte{0, 1, 0, 0, 0, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetWDT2StatusRegOrig := ResetWDT2StatusReg
		defer func() { ResetWDT2StatusReg = ResetWDT2StatusRegOrig }()
                GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetWDT2StatusReg = func(mem []byte) error { return errors.Errorf("Should not have been called") }
		utils.GetMachine = func() (string, error) { return "armv6l", nil }

		res := ExposeRealFlash0OnSecondaryBoot(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "^[^\n]+WDT2 second boot code flag @ 0x0 not active [(]current value = 0x100[)], skipping step\n$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}
	})

	t.Run("Should call ResetWDT2StatusReg() if flag is active", func(t *testing.T) {
		mock_mem = []byte{2, 1, 0, 0, 0, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetWDT2StatusRegOrig := ResetWDT2StatusReg
		defer func() { ResetWDT2StatusReg = ResetWDT2StatusRegOrig }()
                GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetWDT2StatusReg = func(mem []byte) error {
			// Simulate clearing flag on ResetWDT2StatusReg() call
			mem[0] = 0
			return ResetWDT2StatusRegOrig(mem)
		}
		utils.GetMachine = func() (string, error) { return "armv6l", nil }

		res := ExposeRealFlash0OnSecondaryBoot(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+WDT2 second boot code flag @ 0x0 active [(]current value = 0x102[)], clearing flag...\n" +
			"[^\n]+WDT2 second boot code flag cleared, current value @ 0x0 = 0x100\n" +
			"$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

		// Should've written 1 to WDT2_CLR_STATUS_REG_ADDR
		if mock_mem[WDT2_CLR_STATUS_REG_ADDR] != 1 {
			t.Errorf("Expected ResetWDT2StatusRegOrig() to have written 1 to WDT2_CLR_STATUS_REG_ADDR")
		}

	})

	t.Run("Should return error if ResetWDT2StatusReg() isn't able to reset flag", func(t *testing.T) {
		mock_mem = []byte{2, 1, 0, 0, 0, 0, 0, 0}

		var buf bytes.Buffer
		log.SetOutput(&buf)

		ResetWDT2StatusRegOrig := ResetWDT2StatusReg
		defer func() { ResetWDT2StatusReg = ResetWDT2StatusRegOrig }()
                GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()

		ResetWDT2StatusReg = func(mem []byte) error {
			// Simulate flag not clearing on ResetWDT2StatusReg() call
			mem[0] = 2
			return ResetWDT2StatusRegOrig(mem)
		}
		utils.GetMachine = func() (string, error) { return "armv6l", nil }

		res := ExposeRealFlash0OnSecondaryBoot(step.StepParams{})

		expectedError := step.ExitSafeToReboot{Err: errors.Errorf("Unable to clear WDT2 second boot code flag @ 0x0, current value = 0x102")}

		step.CompareTestExitErrors(expectedError, res, t)

		re_expected_log_buffer := "" +
			"^[^\n]+WDT2 second boot code flag @ 0x0 active [(]current value = 0x102[)], clearing flag...\n" +
			"$"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})

	t.Run("Should bail out if not running on AST2400/AST2500", func(t *testing.T) {
		var buf bytes.Buffer
		log.SetOutput(&buf)

                GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()
		utils.GetMachine = func() (string, error) { return "lumpajunka", nil }

		res := ExposeRealFlash0OnSecondaryBoot(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+Remediation handles only AST2400 and AST2500"

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})

	t.Run("Should bail out if running old versions of galaxy cmm", func(t *testing.T) {
		var buf bytes.Buffer
		log.SetOutput(&buf)

		GetMachineOrig := utils.GetMachine
		defer func() { utils.GetMachine = GetMachineOrig }()
		utils.GetMachine = func() (string, error) { return "armv6l", nil }

		utils.GetOpenBMCVersionFromIssueFile = func() (string, error) { return "cmm-v29", nil}

		res := ExposeRealFlash0OnSecondaryBoot(step.StepParams{})

		if res != nil {
			t.Errorf("Expected ExposeRealFlash0OnSecondaryBoot() to return nil, got %v", res)
		}

		re_expected_log_buffer := "" +
			"^[^\n]+Old galaxy cmm version detected .cmm-v29., skipping step..."

		if !regexp.MustCompile(re_expected_log_buffer).Match(buf.Bytes()) {
			t.Errorf("Unexpected log buffer: %v", buf.String())
		}

	})
}
