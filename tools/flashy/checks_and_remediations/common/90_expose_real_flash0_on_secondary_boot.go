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
	"os"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// From ASPEED AST2400/AST1250 A1 Datasheet – V1.3, p524
// WDT30: WDT2 Timeout Status Register
var WDT2_STATUS_REG_ADDR = 0x1e785030
var WDT2_CLR_STATUS_REG_ADDR = 0x1e785034
var MMAP_LEN = 0x1e790000

var Mmap = syscall.Mmap
var Munmap = syscall.Munmap

func init() {
	step.RegisterStep(ExposeRealFlash0OnSecondaryBoot)
}

// On systems where the WDT2 timeout status register's boot source flag is 1,
// flash0 and flash1 devices both point to the same device and writes to flash0
// might give misleading results (as we might get "successful" results while not
// writing to flash0 at all)
// This step detects whenever the flag is active and resets it
func ExposeRealFlash0OnSecondaryBoot(stepParams step.StepParams) step.StepExitError {
	machine, err := utils.GetMachine()
	if err != nil {
		return step.ExitSafeToReboot{Err: errors.Errorf("Unable to fetch machine: %v", err)}
	}

	etcIssueVer, err := utils.GetOpenBMCVersionFromIssueFile()
	if err != nil {
		return step.ExitSafeToReboot{Err: errors.Errorf("Unable to fetch /etc/issue: %v", err)}
	}

	// Bail if not running on AST2400 (armv5tejl) nor AST2500 (armv6l)
	if machine != "armv5tejl" && machine != "armv6l" {
		log.Printf("Remediation handles only AST2400 and AST2500")
		return nil
	}

	// WDT2 reset logic doesn't work on old versions of cmm (it keeps rebooting to the other flash)
	if etcIssueVer == "cmm-v29" {
		log.Printf("Old galaxy cmm version detected (%v), skipping step...", etcIssueVer)
		return nil
	}

	mem, err := MmapDevMemRw()
	if err != nil {
		return step.ExitSafeToReboot{Err: errors.Errorf("Unable to mmap /dev/mem: %v", err)}
	}
	defer Munmap(mem)

	// Check if second boot flag is active...
	wdt2_status_reg := ReadWDT2StatusReg(mem)
	if wdt2_status_reg&0b10 == 0 {
		log.Printf("WDT2 second boot code flag @ 0x%x not active (current value = 0x%x), skipping step", WDT2_STATUS_REG_ADDR, wdt2_status_reg)
		return nil
	}

	// ...and clear it if so.
	log.Printf("WDT2 second boot code flag @ 0x%x active (current value = 0x%x), clearing flag...", WDT2_STATUS_REG_ADDR, wdt2_status_reg)
	err = ResetWDT2StatusReg(mem)
	if err != nil {
		return step.ExitSafeToReboot{Err: err}
	}

	// Log current state of the status register
	wdt2_status_reg = ReadWDT2StatusReg(mem)
	log.Printf("WDT2 second boot code flag cleared, current value @ 0x%x = 0x%x", WDT2_STATUS_REG_ADDR, wdt2_status_reg)
	return nil
}

// Utils
var ReadWDT2StatusReg = func(mem []byte) uint32 {
	wdt2_status_reg_bytes := mem[WDT2_STATUS_REG_ADDR : WDT2_STATUS_REG_ADDR+4]
	return binary.LittleEndian.Uint32(wdt2_status_reg_bytes)
}

var ResetWDT2StatusReg = func(mem []byte) error {
	// On ast2400 systems, clears WDT2 secondary boot flag at WDT2_STATUS_REG_ADDR
	mem[WDT2_CLR_STATUS_REG_ADDR] = 1

	wdt2_status_reg := ReadWDT2StatusReg(mem)
	if wdt2_status_reg&0b10 == 0b10 {
		return errors.Errorf("Unable to clear WDT2 second boot code flag @ 0x%x, current value = 0x%x", WDT2_STATUS_REG_ADDR, wdt2_status_reg)
	}

	return nil
}

var MmapDevMemRw = func() ([]byte, error) {
	// mmaps /dev/mem in RW mode
	f, err := os.OpenFile("/dev/mem", os.O_RDWR, 0)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	mem, err := Mmap(int(f.Fd()), 0, MMAP_LEN, syscall.PROT_READ|syscall.PROT_WRITE, syscall.MAP_SHARED)
	if err != nil {
		return nil, err
	}

	return mem, nil
}
