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
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(dropCaches)
}

const dropCachesFilePath = "/proc/sys/vm/drop_caches"

// dropCaches frees pagecache, dentries and inodes. It is
// equivalent to `echo 3 > /proc/sys/vm/drop_caches`.
func dropCaches(stepParams step.StepParams) step.StepExitError {
	err := fileutils.WriteFileWithTimeout(
		dropCachesFilePath, []byte("3"), 0644, 30*time.Second,
	)
	if err != nil {
		errMsg := errors.Errorf("Failed to write to drop_caches file '%v': %v", dropCachesFilePath, err)
		return step.ExitSafeToReboot{Err: errMsg}
	}
	return nil
}
