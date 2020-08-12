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
	"os"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(fuserKMountRo)
}

// If /mnt/data is smaller in the new image, it must be made read-only
// to prevent the tail of the FIT image from being corrupted.
// Determining shrinkage isn't trivial (data0 is omitted from image
// files for one thing) and details may change over time, so just
// remount every MTD read-only. Reboot is expected to restart the
// killed processes.
func fuserKMountRo(stepParams step.StepParams) step.StepExitError {
	log.Printf("fuser will kill rsyslog and cause errors, turning off syslog logging now")
	// rsyslog will be killed by fuser, direct logs to stderr only.
	// Package logger will attempt to restart syslog in the next step,
	// so this is fine.
	log.SetOutput(os.Stderr)

	writableMountedMTDs, err := devices.GetWritableMountedMTDs()
	if err != nil {
		return step.ExitSafeToReboot{err}
	}

	log.Printf("Writable mounted MTDs: %v", writableMountedMTDs)

	for _, writableMountedMTD := range writableMountedMTDs {
		fuserCmd := []string{"fuser", "-km", writableMountedMTD.Mountpoint}
		_, err, _, _ := utils.RunCommand(fuserCmd, 30)
		if err != nil {
			errMsg := errors.Errorf("Fuser command %v failed: %v", fuserCmd, err)
			return step.ExitSafeToReboot{errMsg}
		}

		remountCmd := []string{"mount", "-o", "remount,ro",
			writableMountedMTD.Device, writableMountedMTD.Mountpoint}
		_, err, _, _ = utils.RunCommandWithRetries(remountCmd, 30, 3, 30)
		if err != nil {
			errMsg := errors.Errorf("Remount command %v failed: %v", remountCmd, err)
			return step.ExitSafeToReboot{errMsg}
		}
	}
	return nil
}
