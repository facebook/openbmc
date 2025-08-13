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
	step.RegisterStep(enableABR)
}

const newBootCmd = "echo Enabling ABR ; otp pb strap o 0x2b 1 ; bootm 20100000"

// Enable automatic boot recovery on morgan800cc so that if uboot on the
// primary chip is wiped or corrupted, the system will boot instead from
// the second chip.
//
// This remediation only meant to do work on first flash during upgrade.  On
// the second (if any) flash of the BMC, ABR will be enabled.
func enableABR(stepParams step.StepParams) step.StepExitError {
	// If the otp command is present, we can assume that ABR has been
	// enabled from the factory (or already set manually for devices
	// already in Meta's fleet).
	for _, path := range []string{"/sbin/otp", "/usr/sbin/otp"} {
		if fileutils.FileExists(path) {
			log.Printf("%v exists - skipping remediation", path)
			return nil
		}
	}

	// Bail out if we are running a Meta supplied BMC image; the BMC
	// has already been upgraded and we assume that ABR is enabled.
	version, err := utils.GetOpenBMCVersionFromIssueFile()
	if err != nil {
		log.Printf("Unable to parse issue file, ignoring: %v", err)
		return nil
	}
	if strings.Contains(version, "morgan800cc-v20") {
		log.Printf("Running a Meta built image - skipping remediation")
		return nil
	}

	// Check to see if bootcmd is already set.  If it has been set,
	// assume that ABR is enabled.
	cmd := []string{"fw_printenv", "bootcmd"}
	_, err, stdout, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		log.Printf("U-Boot environment is inaccessible."+
				" Error code: %v, stderr: %v", err, stderr)
		return nil
	}
	curBootCmd := strings.Replace(strings.TrimSpace(stdout), "bootcmd=", "", 1)
	log.Printf("Current bootcmd: [%v]", curBootCmd)
	if newBootCmd == curBootCmd {
		log.Printf("bootcmd doesn't need to be updated")
		return nil
	}

	// Set new bootcmd and reboot
	cmd = []string{"fw_setenv", "bootcmd", newBootCmd}
	_, err, _, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		log.Printf("fw_setenv doesn't work, ignoring: %v, stderr: %v", err, stderr)
		return nil
	}

	// Force a reboot to pick up new mtdparts.
	errMsg := errors.Errorf("Forcing reboot to enable ABR")
	return step.ExitMustReboot{Err: errMsg}
}
