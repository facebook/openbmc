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
	"fmt"
	"log"
	"strings"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

// const PS1Line = `
// export PS1="[UPGRADE_MODE]$PS1"`

// also used in wall
const motdContents = `
========================
!!!   UPGRADE MODE   !!!
========================

=== PROCEED WITH CAUTION ===

This OpenBMC is now in UPGRADE MODE.
Performing any actions now can crash the upgrade
and brick the device!
`

func init() {
	step.RegisterStep(alertUpgradeMode)
}

// alertUpgradeMode does the following actions:
// - change MOTD
// - wall alert
func alertUpgradeMode(stepParams step.StepParams) step.StepExitError {
	// as these are best-effort, the errors are ignored
	// these won't affect the actual upgrade

	// utils.LogAndIgnoreErr(updatePS1())

	utils.LogAndIgnoreErr(updateMOTD())
	utils.LogAndIgnoreErr(wallAlert())
	return nil
}

// PS1 update to be enabled when there is confirmation
// that there will be no impact on other systems
// func updatePS1() error {
// 	err := fileutils.AppendFile("/etc/profile", PS1Line)
// 	if err != nil {
// 		return errors.Errorf("Warning: Unable to update PS1: %v", err)
// 	}
// 	return nil
// }

var wallAlert = func() error {
	escapedMsg := strings.ReplaceAll(motdContents, "\n", "\\n")
	wallCmd := []string{"bash", "-c", fmt.Sprintf("echo -en \"%v\" | wall", escapedMsg)}
	_, err, _, _ := utils.RunCommand(wallCmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("Warning: `wall` alert failed: %v", err)
	}
	return nil
}

var updateMOTD = func() error {
	if !fileutils.PathExists("/etc/motd.bak") {
		err := fileutils.RenameFile("/etc/motd", "/etc/motd.bak")
		if err != nil {
			log.Printf("Error saving /etc/motd, ignoring: %v", err)
		}
	}
	err := fileutils.WriteFileWithTimeout(
		"/etc/motd", []byte(motdContents), 0644, 5*time.Second,
	)
	if err != nil {
		return errors.Errorf("Warning: MOTD update failed: %v", err)
	}
	return nil
}
