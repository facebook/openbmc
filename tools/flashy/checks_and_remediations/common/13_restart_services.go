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
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(restartServices)
}

func restartServices(stepParams step.StepParams) step.StepExitError {
	var supervisor string

	systemdAvail, err := utils.SystemdAvailable()
	if err != nil {
		errMsg := errors.Errorf("Error checking systemd availability: %v", err)
		return step.ExitSafeToReboot{Err: errMsg}
	}

	if systemdAvail {
		supervisor = "systemctl"
	} else {
		supervisor = "sv"
	}

	log.Printf("Restarting restapi...")
	_, err, _, _ = utils.RunCommand([]string{supervisor, "restart", "restapi"}, 60*time.Second)
	if err != nil {
		log.Printf("Could not restart restapi: %v. "+
			"Ignoring as this is best effort and not critical.",
			err)
	}
	log.Printf("Finished restarting restapi")

	// RestartHealthd() also takes care of petting the watchdog and
	// increasing its timeout (because opening /dev/watchdog requires
	// healthd to be stopped).  If healthd is not in use, perform the
	// watchdog step directly.  The timeout increase stops the BMC
	// rebooting during the following heavyweight steps, like image
	// validation.
	if utils.HealthdExists() {
		log.Printf("Healthd exists, attempting to restart healthd...")
		err = utils.RestartHealthd(true, supervisor)
		if err != nil {
			return step.ExitSafeToReboot{Err: err}
		} else {
			log.Printf("Finished restarting healthd")
		}
	// Linux Foundation uses systemd to pet the watchdog directly.
	} else if utils.IsLFOpenBMC() {
		log.Printf("LF-OpenBMC, letting systemd maintain the watchdog")
	} else {
		utils.PetWatchdog()
	}

	return nil
}
