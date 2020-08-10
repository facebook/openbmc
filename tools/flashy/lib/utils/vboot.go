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
	"strings"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/pkg/errors"
)

type VbootEnforcementType = string

const (
	VBOOT_NONE             VbootEnforcementType = "NONE"
	VBOOT_SOFTWARE_ENFORCE                      = "SOFTWARE_ENFORCE"
	VBOOT_HARDWARE_ENFORCE                      = "HARDWARE_ENFORCE"
)

const vbootUtilPath = "/usr/local/bin/vboot-util"

// Primitive method to check whether the system is a vboot system
// Some firmware versions _may_ have this file but is not vboot,
// but for now it is sufficient to use this check
var IsVbootSystem = func() bool {
	return fileutils.FileExists(vbootUtilPath)
}

var getVbootUtilContents = func() (string, error) {
	if !IsVbootSystem() {
		return "", errors.Errorf("Not a vboot system")
	}

	// due to a bug in vboot-util if it use cached data for rom version it may
	// results in trailing garbage and fail during bytes decode, we need to nuke
	// the cache first as a mitigation
	// this is best-effort, as these files may have already been deleted
	// or may not exist
	LogAndIgnoreErr(fileutils.RemoveFile("/tmp/cache_store/rom_version"))
	LogAndIgnoreErr(fileutils.RemoveFile("/tmp/cache_store/rom_uboot_version"))

	// vboot-util on Tioga Pass 1 v1.9 (and possibly other versions) is a shell
	// script without a shebang line.
	// Check whether it is an ELF file first, if not, add bash in front
	cmd := []string{vbootUtilPath}
	if !fileutils.IsELFFile(vbootUtilPath) {
		// prepend "bash"
		cmd = append([]string{"bash"}, cmd...)
	}

	_, err, stdout, _ := RunCommand(cmd, 30)
	if err != nil {
		return "", errors.Errorf("Unable to get vboot-util info: %v", err)
	}

	return stdout, nil
}

// get the vboot enforcement type of the system
var GetVbootEnforcement = func() (VbootEnforcementType, error) {
	if !IsVbootSystem() {
		return VBOOT_NONE, nil
	}

	// check if "romx" is in procMtdBuf
	procMtdBuf, err := fileutils.ReadFile(ProcMtdFilePath)
	if err != nil {
		return VBOOT_NONE, errors.Errorf("Unable to read '%v': %v",
			ProcMtdFilePath, err)
	}
	if !strings.Contains(string(procMtdBuf), "romx") {
		return VBOOT_NONE, nil
	}

	// check flags in vboot-util
	vbootUtilContents, err := getVbootUtilContents()
	if err != nil {
		return VBOOT_NONE, errors.Errorf("Unable to read vboot-util contents: %v", err)
	}
	if strings.Contains(vbootUtilContents, "Flags hardware_enforce:  0x00") &&
		strings.Contains(vbootUtilContents, "Flags software_enforce:  0x01") {
		return VBOOT_SOFTWARE_ENFORCE, nil
	}
	return VBOOT_HARDWARE_ENFORCE, nil
}
