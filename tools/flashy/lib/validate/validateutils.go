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

package validate

import (
	"log"
	"strings"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

const ubootVersionRegEx = `U-Boot \d+\.\d+ (?P<version>[^\s]+)`

// Deal with images that have changed names, but are otherwise compatible.
// The version strings are free form, so to come up with regexes that safely
// matches all possible formats would be tough. Instead, we use this to do
// substitutions before matching in areVersionsCompatible().
// NB: the values of this mapping CANNOT contain a dash!
var compatibleVersionMapping = map[string]string{"fby2-gpv2": "fbgp2"}

var normalizeVersion = func(ver string) string {
	for k, v := range compatibleVersionMapping {
		ver = strings.Replace(ver, k, v, 1)
	}
	return ver
}

// check compatibility of the image file by comparing the "normalized" build name of
// (1) the /etc/issue file and (2) the image file
// return an error if they do not match
var CheckImageBuildNameCompatibility = func(stepParams utils.StepParams) error {
	if stepParams.Clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing image build name compatibility check =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return nil
	}

	// convenience function to return compatibilty check failed error
	checkFailed := func(err error) error {
		return errors.Errorf("Image build name compatibility check failed: %v. "+
			"Use the '--clowntown' flag if you wish to proceed at the risk of "+
			"bricking the device", err)
	}

	etcIssueVer, err := utils.GetOpenBMCVersionFromIssueFile()

	if err != nil {
		return checkFailed(err)
	}
	log.Printf("OpenBMC Version from /etc/issue: '%v'", etcIssueVer)

	imageFileVer, err := getOpenBMCVersionFromImageFile(stepParams.ImageFilePath)
	if err != nil {
		return checkFailed(err)
	}
	log.Printf("OpenBMC Version from image file '%v': '%v'",
		stepParams.ImageFilePath, imageFileVer)

	// the two build names below are normalized versions
	etcIssueBuildName, err := getNormalizedBuildNameFromVersion(etcIssueVer)
	if err != nil {
		return checkFailed(err)
	}
	log.Printf("OpenBMC (normalized) build name from /etc/issue: '%v'", etcIssueBuildName)

	imageFileBuildName, err := getNormalizedBuildNameFromVersion(imageFileVer)
	if err != nil {
		return checkFailed(err)
	}
	log.Printf("OpenBMC (normalized) name from image file '%v': '%v'",
		stepParams.ImageFilePath, imageFileBuildName)

	// these build names might not match for old versions, as either /etc/isssue
	// or the image file might not be well-formed
	if etcIssueBuildName != imageFileBuildName {
		err := errors.Errorf("OpenBMC versions from /etc/issue ('%v') and image file ('%v')"+
			" do not match!",
			etcIssueBuildName, imageFileBuildName)
		return checkFailed(err)
	}
	return nil
}

// fby2-gpv2-v2019.43.1 -> fbgp2
// yosemite-v1.2 -> yosemite
var getNormalizedBuildNameFromVersion = func(ver string) (string, error) {
	nVer := normalizeVersion(ver)

	buildNameRegEx := `^(?P<buildname>\w+)`
	verMap, err := utils.GetRegexSubexpMap(buildNameRegEx, nVer)
	if err != nil {
		return "", errors.Errorf("Unable to get build name from version '%v' (normalized: '%v'): %v",
			ver, nVer, err)
	}
	return verMap["buildname"], nil
}

// gets OpenBMC version from image file
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: This relies on the U-Boot version string on the image
// there is no guarantee that this will succeed
var getOpenBMCVersionFromImageFile = func(imageFilePath string) (string, error) {
	// mmap the first 512kB of the image file
	imageFileBuf, err := utils.MmapFileRange(
		imageFilePath, 0, 512*1024, syscall.PROT_READ, syscall.MAP_SHARED,
	)
	if err != nil {
		return "", errors.Errorf("Unable to read and mmap image file '%v': %v",
			imageFilePath, err)
	}
	// unmap
	defer utils.Munmap(imageFileBuf)

	imageFileVerMap, err := utils.GetBytesRegexSubexpMap(ubootVersionRegEx, imageFileBuf)
	if err != nil {
		return "", errors.Errorf("Unable to find OpenBMC version in image file '%v': %v",
			imageFilePath, err)
	}
	version := imageFileVerMap["version"]

	return version, nil
}
