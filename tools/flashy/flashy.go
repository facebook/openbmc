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

package main

import (
	"flag"
	"log"
	"os"

	"github.com/facebook/openbmc/tools/flashy/install"
	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

var (
	installFlag   = flag.Bool("install", false, "Install flashy")
	imageFilePath = flag.String("imagepath", "/opt/upgrade/image", "Path to image file")
	deviceID      = flag.String("device", "", "Device ID (e.g. mtd:flash0")
	clowntown     = flag.Bool("clowntown", false, "Clowntown mode (WARNING: RISK OF BRICKING DEVICE - WARRANTIES OFF)")
)

// exit if not empty
func failIfFlagEmpty(flagName, value string) {
	if len(value) == 0 {
		log.Fatalf("%v flag must be specified. Use `--help` for a guide", flagName)
	}
}

func main() {
	flag.Parse()

	if *clowntown {
		log.Printf(`===== WARNING: CLOWNTOWN MODE ENABLED =====
THERE IS RISK OF BRICKING THIS DEVICE!
WARRANTIES OFF`)
	}

	binName := fileutils.SanitizeBinaryName(os.Args[0])

	stepParams := step.StepParams{
		Install:       *installFlag,
		ImageFilePath: *imageFilePath,
		DeviceID:      *deviceID,
		Clowntown:     *clowntown,
	}

	// install
	if stepParams.Install {
		install.Install()
		return
	}

	// at this point, imageFilePath and deviceID are required to be non empty
	failIfFlagEmpty("imagepath", *imageFilePath)
	failIfFlagEmpty("device", *deviceID)

	if _, exists := step.StepMap[binName]; !exists {
		log.Fatalf("Unknown binary '%v'", binName)
	}

	log.Printf("Starting: %v", binName)
	err := step.StepMap[binName](stepParams)
	if err != nil {
		step.HandleStepError(err)
	}
	log.Printf("Finished: %v", binName)
}
