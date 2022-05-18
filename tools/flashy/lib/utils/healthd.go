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

package utils

import (
	"log"
	"time"

	"github.com/Jeffail/gabs"
	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/pkg/errors"
)

const healthdConfigFilePath = "/etc/healthd-config.json"

/*
Known Platforms with healthd-config.json:
fbal
fbep
fbsp
fbtp
fbttn
fby2
fby3
lightning
minilaketb
yosemite
*/

// BmcMemUtil is a struct for parsing the healthd-config.json.
type BmcMemUtil struct {
	Threshold []BmcMemThres `json:"threshold"`
}

// BmcMemThres is a struct for parsing the healthd-config.json.
type BmcMemThres struct {
	Value   float32  `json:"value"`
	Actions []string `json:"action"`
}

// HealthdExists checks if healthd exists in this system by
// checking existence of the healthd config file.
var HealthdExists = func() bool {
	return fileutils.FileExists(healthdConfigFilePath)
}

// GetHealthdConfig gets the healthd config in a JSON container.
var GetHealthdConfig = func() (*gabs.Container, error) {
	buf, err := fileutils.ReadFile(healthdConfigFilePath)
	if err != nil {
		return nil, errors.Errorf("Unable to open healthd-config.json: %v", err)
	}
	healthdConfig, err := gabs.ParseJSON(buf)
	if err != nil {
		return nil, errors.Errorf("Unable to parse healthd-config.json: %v", err)
	}
	return healthdConfig, nil
}

// HealthdRemoveMemUtilRebootEntryIfExists removes the "reboot" entry that will trigger system reboot after
// mem util reaches a threshold. This is to prevent healthd reboots during flashing.
// If the "reboot" entry exists, remove it and write back to the file.
func HealthdRemoveMemUtilRebootEntryIfExists(h *gabs.Container) error {
	rebootExists := false
	thresholds, err := h.Path("bmc_mem_utilization.threshold").Children()
	if err != nil {
		return errors.Errorf("Can't get 'bmc_mem_utilization.threshold' entry in healthd-config %v",
			h.String())
	}
	for _, threshold := range thresholds {
		actions, err := threshold.Path("action").Children()
		if err != nil {
			return errors.Errorf("Can't get 'action' entry in healthd-config %v", h.String())
		}
		for i, action := range actions {
			if action.Data().(string) == "reboot" {
				threshold.ArrayRemove(i, "action")
				rebootExists = true
			}
		}
	}

	if rebootExists {
		err = HealthdWriteConfigToFile(h)
		if err != nil {
			return err
		}
		log.Printf("Healthd reboot action exists and was removed.")
	}
	return nil
}

// HealthdWriteConfigToFile writes back into /etc/healthd-config.json.
func HealthdWriteConfigToFile(h *gabs.Container) error {
	buf := []byte(h.StringIndent("", "  "))

	err := fileutils.WriteFileWithTimeout(healthdConfigFilePath, buf, 0644, 10*time.Second)
	if err != nil {
		return errors.Errorf("Unable to write healthd config to file: %v",
			err)
	}

	return nil
}

// RestartHealthd restarts the healthd process.
var RestartHealthd = func(wait bool, supervisor string) error {
	if !fileutils.PathExists("/etc/sv/healthd") {
		return errors.Errorf("Error restarting healthd: '/etc/sv/healthd' does not exist")
	}

	// ignore healthd not running (seen on fby3-v2020.33.1)
	code, err, _, _ := RunCommand([]string{supervisor, "status", "healthd"}, 60*time.Second)
	if err != nil || code != 0 {
		log.Printf("healthd doesn't seem to be running.  Wait for 30s to " +
			"ensure watchdog is stable.")
		Sleep(30 * time.Second)
		return nil
	}

	// stop healthd, forcing it to close /dev/watchdog.  ignore errors.
	RunCommand([]string{supervisor, "stop", "healthd"}, 60*time.Second)

	// now try to pet the watchdog and extend its timeout.
	PetWatchdog()

	// re-start healthd
	_, err, _, _ = RunCommand([]string{supervisor, "start", "healthd"}, 60*time.Second)

	// healthd is petting watchdog, if something goes wrong and it doesn't do so
	// after restart it may hard-reboot the system - it's better to be safe
	// than sorry here, let's wait 30s before proceeding
	if wait {
		log.Printf("Sleeping for 30s after healthd restart to make sure " +
			"healthd is stable.")
		Sleep(30 * time.Second)
	}

	// even in the error case, proceed only after waiting for the
	// watchdog.  an error message is logged by RunCommand() on failure,
	// so this does not cause loss of signal.
	return err
}
