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
	"os/signal"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/install"
	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/logger"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/validate/image"
)

var (
	// modes
	installFlag = flag.Bool("install", false, "Install flashy")
	checkImage  = flag.Bool("checkimage", false,
		"Validate image partitions (`imagepath` must be specified). Available on non-OpenBMC systems. "+
			"If `device` is specified the image file size will be validated against the size of the device.")
	// step params
	imageFilePath = flag.String("imagepath", "", "Path to image file")
	deviceID      = flag.String("device", "", "Device ID (e.g. mtd:flash0)")
	clowntown     = flag.Bool("clowntown", false, "Clowntown mode (WARNING: RISK OF BRICKING DEVICE - WARRANTIES OFF)")
)

// exit if not empty
func failIfFlagEmpty(flagName, value string) {
	if len(value) == 0 {
		log.Fatalf("`%v` argument must be specified. Use `--help` for a guide",
			flagName)
	}
}

// ignoreSignals prevents things like dropped SSH connections or ^C from
// interrupting flashy. This is important so we don't have dropped connections
// leaving the device in a bricked state.
func ignoreSignals() {
	var signalsToIgnore = []os.Signal{
		syscall.SIGHUP,
		syscall.SIGINT,
		syscall.SIGTERM,
	}

	for _, sig := range signalsToIgnore {
		signal.Ignore(sig)
	}
}

func main() {
	// set logger output to CustomLogger to log to both syslog and stderr
	log.SetOutput(logger.CustomLogger)

	binName := fileutils.SanitizeBinaryName(os.Args[0])

	// check if it's a utilities function
	if _, exists := step.UtilitiesMap[binName]; exists {
		err := step.UtilitiesMap[binName](os.Args)
		if err != nil {
			log.Fatalf("%v failed: %v", binName, err)
		}
		return
	}

	flag.Parse()

	stepParams := step.StepParams{
		ImageFilePath: *imageFilePath,
		DeviceID:      *deviceID,
		Clowntown:     *clowntown,
	}

	if stepParams.Clowntown {
		log.Printf(`===== WARNING: CLOWNTOWN MODE ENABLED =====
THERE IS RISK OF BRICKING THIS DEVICE!
WARRANTIES OFF`)
	}

	// install
	if *installFlag {
		log.Println("Installing flashy...")
		install.Install(stepParams)
		log.Println("Finished installing flashy")
		return
	}

	// validate image
	if *checkImage {
		log.Printf("Validating image...")
		failIfFlagEmpty("imagepath", stepParams.ImageFilePath)

		err := image.ValidateImageFile(stepParams.ImageFilePath, stepParams.DeviceID)
		if err != nil {
			log.Fatalf("Image '%v' failed validation: %v",
				stepParams.ImageFilePath, err)
		}

		log.Printf("Finished validating image")
		return
	}

	// code below this point is for a symlink-ed step
	// (e.g. flash_procedure/flash_wedge100)

	// ignore signals for steps
	ignoreSignals()

	// at this point, imageFilePath and deviceID
	// are required to be non empty
	failIfFlagEmpty("imagepath", stepParams.ImageFilePath)
	failIfFlagEmpty("device", stepParams.DeviceID)

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
