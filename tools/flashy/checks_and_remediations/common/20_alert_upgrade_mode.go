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

// change PS1 to [UPGRADE_MODE]u@\h:\w\$
// change MOTD
// wall alert
package common

import (
	"fmt"
	"log"
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

const PS1Line = `
export PS1="[UPGRADE_MODE]$PS1"`

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
	utils.RegisterStepEntryPoint(alertUpgradeMode)
}

func alertUpgradeMode(imageFilePath, deviceID string) utils.StepExitError {
	// as these are best-effort, the errors are ignored
	// these won't affect the actual upgrade
	logErr := func(err error) {
		if err != nil {
			log.Printf("%v", err)
		}
	}

	// PS1 update to be enabled when there is confirmation
	// that there will be no impact on other systems
	// logErr(updatePS1())

	logErr(updateMOTD())
	logErr(wallAlert())
	return nil
}

func updatePS1() error {
	err := utils.AppendFile("/etc/profile", PS1Line)
	if err != nil {
		return errors.Errorf("Warning: Unable to update PS1: %v", err)
	}
	return nil
}

func wallAlert() error {
	escapedMsg := strings.ReplaceAll(motdContents, "\n", "\\n")
	wallCmd := []string{"bash", "-c", fmt.Sprintf("echo -en \"%v\" | wall", escapedMsg)}
	_, err, _, _ := utils.RunCommand(wallCmd, 30)
	if err != nil {
		return errors.Errorf("Warning: `wall` alert failed: %v", err)
	}
	return nil
}

func updateMOTD() error {
	err := utils.WriteFile("/etc/motd", []byte(motdContents), 0644)
	if err != nil {
		return errors.Errorf("Warning: MOTD update failed, `wall` alert will not proceed: %v", err)
	}
	return nil
}
