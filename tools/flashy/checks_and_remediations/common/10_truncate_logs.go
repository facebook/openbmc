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
	"log"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(truncateLogs)
}

// these files will be deleted
var deleteLogFilePatterns []string = []string{
	"/var/log/message.*",
	"/var/log/*.log.*",
	"/var/log/*.gz",
}

// these files will be truncated
var truncateLogFilePatterns []string = []string{
	"/var/log/messages",
}

func truncateLogs(stepParams step.StepParams) step.StepExitError {
	logFilesToDelete, err := fileutils.GlobAll(deleteLogFilePatterns)
	if err != nil {
		errMsg := errors.Errorf("Unable to resolve file patterns '%v': %v", deleteLogFilePatterns, err)
		return step.ExitSafeToReboot{errMsg}
	}

	for _, f := range logFilesToDelete {
		log.Printf("Removing '%v'", f)
		err := fileutils.RemoveFile(f)
		if err != nil {
			errMsg := errors.Errorf("Unable to remove log file '%v': %v", f, err)
			return step.ExitSafeToReboot{errMsg}
		}
	}

	logFilesToTruncate, err := fileutils.GlobAll(truncateLogFilePatterns)
	if err != nil {
		errMsg := errors.Errorf("Unable to resolve file patterns '%v': %v", deleteLogFilePatterns, err)
		return step.ExitSafeToReboot{errMsg}
	}

	for _, f := range logFilesToTruncate {
		log.Printf("Truncating '%v'", f)
		err := fileutils.TruncateFile(f, 0)
		if err != nil {
			errMsg := errors.Errorf("Unable to truncate log file '%v': %v", f, err)
			return step.ExitSafeToReboot{errMsg}
		}
	}
	return nil
}
