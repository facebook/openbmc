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
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/flash/flashutils/devices"
	"github.com/facebook/openbmc/tools/flashy/lib/logger"
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
	// rsyslog will be killed by fuser, direct logs to stderr only.
	// Try to restart syslog at the end of this function, but if this fails
	// it is fine because syslog will try to be restarted in the start of the next
	// step (as the CustomLogger gets the syslog writer)
	log.Printf("fuser will kill rsyslog and cause errors, turning off syslog logging now")
	log.SetOutput(os.Stderr)
	defer func() {
		logger.StartSyslog()
	}()

	writableMountedMTDs, err := devices.GetWritableMountedMTDs()
	if err != nil {
		return step.ExitSafeToReboot{err}
	}

	log.Printf("Writable mounted MTDs: %v", writableMountedMTDs)

	for _, writableMountedMTD := range writableMountedMTDs {
		fuserCmd := []string{"fuser", "-km", writableMountedMTD.Mountpoint}
		_, err, _, _ := utils.RunCommand(fuserCmd, 30*time.Second)
		// we ignore the error in fuser, as this might be thrown because nothing holds
		// a file open on the file system (because of delayed log file open or because
		// rsyslogd is not running for some reason).
		// Retrying remount with a delay below should work around surviving processes.
		utils.LogAndIgnoreErr(err)

		remountCmd := []string{"mount", "-o", "remount,ro",
			writableMountedMTD.Device, writableMountedMTD.Mountpoint}
		_, err, _, _ = utils.RunCommandWithRetries(remountCmd, 30*time.Second, 3, 30*time.Second)
		if err != nil {
			errMsg := errors.Errorf("Remount command %v failed: %v", remountCmd, err)
			return step.ExitSafeToReboot{errMsg}
		}
	}
	return nil
}
