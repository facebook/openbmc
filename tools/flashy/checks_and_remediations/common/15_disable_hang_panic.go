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
	"errors"
	"fmt"
	"log"
	"os"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
)

func init() {
	step.RegisterStep(disableHangPanic)
}

const disableHangPanicFilePath = "/proc/sys/kernel/hung_task_panic"

// disableHangPanic disables the "hung_task_panic".
func disableHangPanic(stepParams step.StepParams) step.StepExitError {
	if _, err := os.Stat(disableHangPanicFilePath); errors.Is(err, os.ErrNotExist) {
		log.Printf("%v does not exist. not proceeding with disableHungPanic", disableHangPanicFilePath)
		return nil
	}
	err := fileutils.WriteFileWithTimeout(
		disableHangPanicFilePath, []byte("0"), 0644, 30*time.Second,
	)
	if err != nil {
		errMsg := fmt.Errorf("Failed to write to hang_task_panic file '%v': %v", disableHangPanicFilePath, err)
		return step.ExitSafeToReboot{Err: errMsg}
	}
	return nil
}
