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
	"encoding/json"
	"log"

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

// partial healthd config required for JSON unmarshaling
type HealthdConfig struct {
	BmcMemUtilization BmcMemUtil `json:"bmc_mem_utilization"`
}

type BmcMemUtil struct {
	Threshold []BmcMemThres `json:"threshold"`
}

type BmcMemThres struct {
	Value   float32  `json:"value"`
	Actions []string `json:"action"`
}

// returns if healthd exists in this system
// checks existence of the healthd config file
var HealthdExists = func() bool {
	return FileExists(healthdConfigFilePath)
}

// reads from healthd-config.json and tries to unmarshal and return
// the healthdConfig
var GetHealthdConfig = func() (*HealthdConfig, error) {
	var healthdConfig HealthdConfig
	if !HealthdExists() {
		return nil, errors.Errorf("Unable to find healthd-config.json file")
	}
	buf, err := ReadFile(healthdConfigFilePath)
	if err != nil {
		return nil, errors.Errorf("Unable to open healthd config: %v", err)
	}
	err = json.Unmarshal(buf, &healthdConfig)
	if err != nil {
		return nil, errors.Errorf("Unable to unmarshal healthd-config.json: %v", err)
	}
	return &healthdConfig, nil
}

// get the healthd reboot threshold percentage
// healthd config specifies a mem utlization threshold (e.g. 80%, 95%)
// that will trigger system reboot
func (h *HealthdConfig) GetRebootThresholdPercentage() float32 {
	thresholds := h.BmcMemUtilization.Threshold
	for _, threshold := range thresholds {
		if StringFind("reboot", threshold.Actions) != -1 {
			log.Printf("Found healthd reboot threshold: %v percent", threshold.Value)
			return threshold.Value
		}
	}
	// there is no guarantee that there is a reboot action
	// default to 100 if so
	log.Printf("No healthd reboot threshold found, defaulting to 100 percent")
	return 100
}
