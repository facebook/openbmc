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

	"github.com/facebook/openbmc/common/recipes-utils/flashy/files/utils"
	"github.com/pkg/errors"
)

func init() {
	utils.RegisterStepEntryPoint(restartServices)
}

func restartServices(imageFilePath, deviceID string) utils.StepExitError {
	var supervisor string

	systemdAvail, err := utils.SystemdAvailable()
	if err != nil {
		errMsg := errors.Errorf("Error checking systemd availability: %v", err)
		return utils.ExitSafeToReboot{errMsg}
	}

	if systemdAvail {
		supervisor = "systemctl"
	} else {
		supervisor = "sv"
	}

	log.Printf("Restarting restapi...")
	_, err, _, _ = utils.RunCommand([]string{supervisor, "restart", "restapi"}, 60)
	if err != nil {
		errMsg := errors.Errorf("Could not restart restapi: %v", err)
		return utils.ExitSafeToReboot{errMsg}
	}
	log.Printf("Finished restarting restapi")

	if utils.HealthdExists() {
		log.Printf("Healthd exists, attempting to restart healthd...")
		err = utils.RestartHealthd(false, supervisor, time.Sleep)
		if err != nil {
			errMsg := errors.Errorf("Could not restart healthd: %v", err)
			return utils.ExitSafeToReboot{errMsg}
		}
		log.Printf("Finished restarting healthd")
	}

	return nil
}
