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

package remediations_grandteton

import (
	"encoding/json"
	"log"
	"regexp"
	"strconv"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/facebook/openbmc/tools/flashy/lib/validate"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(ensureCompatibleMBCPLDVersion)
}

type FWUtilVersionJson struct {
	COMPONENT        string
	FRU              string
	PRETTY_COMPONENT string
	VERSION          string
}

var versionRegex = regexp.MustCompile("^grandteton-(v[0-9]+[.][0-9]+[.][0-9]+)$")

var GetOpenBMCVersionFromImageFile = validate.GetOpenBMCVersionFromImageFile

// ensureCompatibleMBCPLDVersion checks whether the MB CPLD version is compatible with the image being
// flashed
func ensureCompatibleMBCPLDVersion(stepParams step.StepParams) step.StepExitError {
	// Fetch current MB CPLD version from fw-util
	mbCpldVersion, stepErr := fwutilGetMbCpldVersion()
	if stepErr != nil {
		return stepErr
	}

	// Fetch BMC version from image file
	imageFileVer, err := GetOpenBMCVersionFromImageFile(stepParams.ImageFilePath)
	if err != nil {
		return step.ExitSafeToReboot{
			Err: err,
		}
	}

	if !versionRegex.MatchString(imageFileVer) {
		return step.ExitSafeToReboot{
			Err: errors.Errorf("Image version '%v' does not match expected format %v", imageFileVer, versionRegex),
		}
	}

	// S365492: Images newer than v2024.11.1 require MB CPLD version 20010 or greater, otherwise sensor data
	// becomes unavailable (T175831581). Booting older images with the new CPLD version should be safe.
	if imageFileVer >= "grandteton-v2024.11.1" && mbCpldVersion < 20010 {
		return step.ExitSafeToReboot{
			Err: errors.Errorf("S365492: Image version ('%v') requires MB CPLD version 20010 or greater (current MB CPLD version is %d)", imageFileVer, mbCpldVersion),
		}
	}

	log.Printf("MB CPLD version %d is compatible with image version %v", mbCpldVersion, imageFileVer)

	return nil
}

func fwutilGetMbCpldVersion() (int, step.StepExitError) {
	// Run fw-util
	fwutilCmd := []string{
		"fw-util", "mb", "--version-json", "mb_cpld",
	}
	exitCode, err, stdout, stderr := utils.RunCommand(fwutilCmd, 1800*time.Second)

	if err != nil {
		return 0, step.ExitSafeToReboot{
			Err: errors.Errorf("`fw-util mb --version-json mb_cpld` failed with exit code %d, stderr: %v, stdout: %v", exitCode, stderr, stdout),
		}
	}

	var fwutilVersionJson []FWUtilVersionJson
	if err := json.Unmarshal([]byte(stdout), &fwutilVersionJson); err != nil {
		return 0, step.ExitSafeToReboot{
			Err: errors.Errorf("failed to parse `fw-util mb --version-json mb_cpld` output: %v", err),
		}
	}

	if len(fwutilVersionJson) != 1 {
		return 0, step.ExitSafeToReboot{
			Err: errors.Errorf("`fw-util mb --version-json mb_cpld` returned %v entries", len(fwutilVersionJson)),
		}
	}

	mbCpldVersion, err := strconv.Atoi(fwutilVersionJson[0].VERSION)
	if err != nil {
		return 0, step.ExitSafeToReboot{
			Err: errors.Errorf("Cannot parse `fw-util mb --version-json mb_cpld` version as integer: %v", fwutilVersionJson),
		}
	}

	return mbCpldVersion, nil
}
