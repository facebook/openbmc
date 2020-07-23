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
	"regexp"
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

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
