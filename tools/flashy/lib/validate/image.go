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
	"regexp"
	"strings"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

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

// check compatibility of images based on OpenBMC build name
// obtained from (1) /etc/issue file and (2) image file (after normalization).
// TODO:- introduce --force flag and allow forcing
var IsImageBuildNameCompatible = func(imageFilePath string) bool {
	log.Printf("Checking issue file and image file OpenBMC version compatibility...")
	issueVer, err := getOpenBMCVersionFromIssueFile()
	if err != nil {
		log.Printf("%v", err)
		return false
	}
	log.Printf("Issue file OpenBMC Version: '%v'", issueVer)
	imageVer, err := getOpenBMCVersionFromImageFile(imageFilePath)
	if err != nil {
		log.Printf("%v", err)
		return false
	}
	log.Printf("Image file OpenBMC Version: '%v'", imageVer)
	return areVersionsCompatible(issueVer, imageVer)
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

// check compatibility of OpenBMC version strings by comparing
// the build name part AFTER normalizing
var areVersionsCompatible = func(issueVer, imageVer string) bool {
	issueBuildName, err := getNormalizedBuildNameFromVersion(issueVer)
	if err != nil {
		log.Printf("%v", err)
		return false
	}
	imageBuildName, err := getNormalizedBuildNameFromVersion(imageVer)
	if err != nil {
		log.Printf("%v", err)
		return false
	}
	return issueBuildName == imageBuildName
}

// get OpenBMC version from /etc/issue
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: There is no guarantee that /etc/issue is well-formed
// in old images
var getOpenBMCVersionFromIssueFile = func() (string, error) {
	versionRegEx := `^OpenBMC Release (?P<version>[^\s]+)`
	version := ""

	etcIssueBuf, err := utils.ReadFile("/etc/issue")
	if err != nil {
		return version, errors.Errorf("Error reading /etc/issue: %v", err)
	}

	etcIssueMap, err := utils.GetRegexSubexpMap(
		versionRegEx, string(etcIssueBuf))

	if err != nil {
		// does not match regex
		return version,
			errors.Errorf("Unable to get version from /etc/issue: %v", err)
	}

	version = etcIssueMap["version"]

	return version, nil
}

// gets OpenBMC version from image file
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: This relies on the U-Boot version string on the image
// there is no guarantee that this will succeed
var getOpenBMCVersionFromImageFile = func(imageFilePath string) (string, error) {
	version := ""
	versionRegEx := regexp.MustCompile(`U-Boot \d+\.\d+ (?P<version>[^\s]+)`)

	// mmap the first 512kB of the image file
	imageFileBuf, err := utils.MmapFileRange(
		imageFilePath, 0, 512*1024, syscall.PROT_READ, syscall.MAP_SHARED,
	)
	if err != nil {
		return version, errors.Errorf("Unable to read and mmap image file '%v': %v",
			imageFilePath, err)
	}
	// unmap
	defer utils.Munmap(imageFileBuf)

	matches := versionRegEx.FindSubmatch(imageFileBuf)
	if len(matches) < 2 {
		return version, errors.Errorf("Unable to find OpenBMC version in image file '%v'",
			imageFilePath)
	}
	// matches must have 2 entries, first one is empty, second one contains the version
	version = string(matches[1])

	return version, nil
}
