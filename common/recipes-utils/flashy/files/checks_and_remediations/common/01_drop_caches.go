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

package common

import (
	"github.com/facebook/openbmc/tree/helium/common/recipes-utils/flashy/files/utils"
	"github.com/pkg/errors"
)

func init() {
	utils.RegisterStepEntryPoint(dropCaches)
}

const dropCachesFilePath = "/proc/sys/vm/drop_caches"

// frees pagecache, dentries and inodes
// equivalent to `echo 3 > /proc/sys/vm/drop_caches`
func dropCaches(imageFilePath, deviceID string) utils.StepExitError {
	err := utils.WriteFile(dropCachesFilePath, []byte("3"), 0644)
	if err != nil {
		errMsg := errors.Errorf("Failed to write to drop_caches file '%v': %v", dropCachesFilePath, err)
		return utils.ExitSafeToReboot{errMsg}
	}
	return nil
}
