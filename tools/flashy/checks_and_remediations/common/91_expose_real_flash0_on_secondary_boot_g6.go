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
	"encoding/binary"
	"log"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// From ASPEED AST2600 A3 Datasheet v1.3, p261
// FMC64: FMC_WDT2: Control/Status Register for Alternate Boot
var FMC_WDT2 = 0x1E620064
const FMC_WDT2_CLEAR_VAL = 0xea
const FMC_WDT2_CLEAR_OFF = 2

func init() {
	step.RegisterStep(ExposeRealFlash0OnSecondaryBootG6)
}

// On systems where the WDT2 timeout status register's boot source flag is 1,
// flash0 and flash1 devices both point to the same device and writes to flash0
// might give misleading results (as we might get "successful" results while not
// writing to flash0 at all)
// This step detects whenever the flag is active and resets it
func ExposeRealFlash0OnSecondaryBootG6(stepParams step.StepParams) step.StepExitError {
	machine, err := utils.GetMachine()
	if err != nil {
		return step.ExitSafeToReboot{Err: errors.Errorf("Unable to fetch machine: %v", err)}
	}

	// Bail if not running on AST2600
	if machine != "armv7l" {
		log.Printf("Remediation handles only AST2600")
		return nil
	}

	mem, err := MmapDevMemRw()
	if err != nil {
		return step.ExitSafeToReboot{Err: errors.Errorf("Unable to mmap /dev/mem: %v", err)}
	}
	defer syscall.Munmap(mem)

	// Check if second boot flag is active...
	fmc_wdt2 := ReadFMC_WDT2(mem)
	if fmc_wdt2&0x10 == 0 {
		log.Printf("WDT2 second boot code flag @ 0x%x not active (current value = 0x%x), skipping step", FMC_WDT2, fmc_wdt2)
		return nil
	}

	// ...and clear it if so.
	log.Printf("WDT2 second boot code flag @ 0x%x active (current value = 0x%x), clearing flag...", FMC_WDT2, fmc_wdt2)
	err = ResetFMC_WDT2(mem)
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}

	// Log current state of the status register
	fmc_wdt2 = ReadFMC_WDT2(mem)
	log.Printf("WDT2 second boot code flag cleared, current value @ 0x%x = 0x%x", FMC_WDT2, fmc_wdt2)
	return nil
}

// Utils
var ReadFMC_WDT2 = func(mem []byte) uint32 {
	fmc_wdt2_bytes := mem[FMC_WDT2 : FMC_WDT2+4]
	return binary.LittleEndian.Uint32(fmc_wdt2_bytes)
}

var ResetFMC_WDT2 = func(mem []byte) error {
	// On ast2600 systems, clears WDT2 secondary boot flag at FMC_WDT2
	mem[FMC_WDT2 + FMC_WDT2_CLEAR_OFF] = FMC_WDT2_CLEAR_VAL

	fmc_wdt2 := ReadFMC_WDT2(mem)
	if fmc_wdt2&0x10 == 0x10 {
		return errors.Errorf("Unable to clear WDT2 second boot code flag @ 0x%x, current value = 0x%x", FMC_WDT2, fmc_wdt2)
	}

	return nil
}
