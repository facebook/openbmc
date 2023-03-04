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
	"log"
	"strings"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(ensureFlashAvailable)
}

const bootargsAppend = "mtdparts=spi0.0:32M@0x0(flash0),0x20000@0x60000(env)"

// A check for very old OpenBMC images where the "flash0" partition is not
// exposed.  We need to adjust the bootargs and reboot the BMC.
//
// Only very old images exhibit this bug and all have a 32MB SPI chip with
// the same layout (wedge/wedge100/galaxy100).
//
// It's possible for "flash0" to be invisible on other platforms (e.g. 
// minipack) because of a kernel driver bug but in that case the "env"
// partition will also be invisible: we won't be able to make any changes
// via fw_setenv here.
//
// This remediation only meant to do work on first flash during upgrade.  On
// the second (if any) flash of the BMC, a new kernel will be running and
// the correct devices will be visibile.
//
// This is best effort: silently fail if nothing can be done and let the
// flash stage notice that no chip is available.
func ensureFlashAvailable(stepParams step.StepParams) step.StepExitError {
	// Doesn't affect any vboot platforms..
	if fileutils.FileExists("/usr/local/bin/vboot-util") {
		log.Printf("vboot platform, skipping flash availability check")
		return nil
	}

	// LF systems don't have this issue.
	if utils.IsLFOpenBMC() {
		log.Printf("LF-OpenBMC, skipping flash availability check")
		return nil
	}

	// Bail out if the partition is already visble.
	cmd := []string{"grep", "flash0", utils.ProcMtdFilePath}
	_, err, stdout, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err == nil && strings.Contains(stdout, "flash0") {
		log.Printf("Good: flash0 is available")
		return nil
	}

	// Check for mtdparts aleady being set.
	cmd = []string{"fw_printenv", "bootargs"}
	_, err, stdout, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		if strings.Contains(stderr, "Cannot access MTD device") {
			errMsg := errors.Errorf(
				"U-Boot environment is inaccessible: broken flash chip?" +
				" Error code: %v, stderr: %v", err, stderr)
			return step.ExitBadFlashChip{Err: errMsg}
		}
		log.Printf("fw_printenv doesn't work: %v, stderr: %v", err, stderr)
		return nil
	}

	// Override mtdparts on next boot.
	bootargs := strings.Replace(strings.TrimSpace(stdout), "bootargs=", "", 1)
	log.Printf("Current bootargs: [%v]", bootargs)
	if !strings.Contains(stdout, bootargsAppend) {
		bootargs = bootargs + " " + bootargsAppend
		cmd = []string{"fw_setenv", "bootargs", bootargs}
		_, err, stdout, stderr = utils.RunCommand(cmd, 30*time.Second)
		if err != nil {
			log.Printf("fw_setenv doesn't work: %v, stderr: %v", err, stderr)
			return nil
		}
	}
	log.Printf("New bootargs: [%v]", bootargs)

	// Force a reboot to pick up new mtdparts.
	errMsg := errors.Errorf("Forcing reboot for new bootargs to take effect")
	return step.ExitMustReboot{Err: errMsg}
}
