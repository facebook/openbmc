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
	"syscall"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

const ubootVersionRegEx = `U-Boot \d+\.\d+ (?P<version>[^\s]+)`

// CheckImageBuildNameCompatibility checks compatibility of the image file by comparing the "normalized" build name of
// (1) the /etc/issue file and (2) the image file
// return an error if they do not match
var CheckImageBuildNameCompatibility = func(imageFilePath string) error {
	etcIssueVer, err := utils.GetOpenBMCVersionFromIssueFile()
	if err != nil {
		return err
	}
	log.Printf("OpenBMC Version from /etc/issue: '%v'", etcIssueVer)

	imageFileVer, err := getOpenBMCVersionFromImageFile(imageFilePath)
	if err != nil {
		return err
	}
	log.Printf("OpenBMC Version from image file '%v': '%v'",
		imageFilePath, imageFileVer)

	// the two build names below are normalized versions
	etcIssueBuildName, err := utils.GetNormalizedBuildNameFromVersion(etcIssueVer)
	if err != nil {
		return err
	}
	log.Printf("OpenBMC (normalized) build name from /etc/issue: '%v'", etcIssueBuildName)

	imageFileBuildName, err := utils.GetNormalizedBuildNameFromVersion(imageFileVer)
	if err != nil {
		return err
	}
	log.Printf("OpenBMC (normalized) name from image file '%v': '%v'",
		imageFilePath, imageFileBuildName)

	// these build names might not match for old versions, as either /etc/isssue
	// or the image file might not be well-formed
	if etcIssueBuildName != imageFileBuildName {
		err := errors.Errorf("OpenBMC versions from /etc/issue ('%v') and image file ('%v')"+
			" do not match!",
			etcIssueBuildName, imageFileBuildName)
		return err
	}
	return nil
}

// getOpenBMCVersionFromImageFile gets OpenBMC version from the image file.
// examples: fbtp-v2020.09.1, wedge100-v2020.07.1
// WARNING: This relies on the U-Boot version string on the image
// there is no guarantee that this will succeed
var getOpenBMCVersionFromImageFile = func(imageFilePath string) (string, error) {
	// mmap the first 512kB of the image file
	imageFileBuf, err := fileutils.MmapFileRange(
		imageFilePath, 0, 512*1024, syscall.PROT_READ, syscall.MAP_SHARED,
	)
	if err != nil {
		return "", errors.Errorf("Unable to read and mmap image file '%v': %v",
			imageFilePath, err)
	}
	// unmap
	defer fileutils.Munmap(imageFileBuf)

	imageFileVerMap, err := utils.GetBytesRegexSubexpMap(ubootVersionRegEx, imageFileBuf)
	if err != nil {
		return "", errors.Errorf("Unable to find OpenBMC version in image file '%v': %v",
			imageFilePath, err)
	}
	version := imageFileVerMap["version"]

	return version, nil
}
