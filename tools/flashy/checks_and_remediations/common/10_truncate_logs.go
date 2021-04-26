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
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(truncateLogs)
}

// these files will be deleted
var deleteLogFilePatterns []string = []string{
	"/tmp/rest.log.?",
	"/var/log/messages.*",
	"/var/log/*.log.*",
	"/var/log/*.gz",
}

// these files will be truncated
var truncateLogFilePatterns []string = []string{
	"/tmp/rest.log",
	"/var/log/messages",
	"/var/log/*.log",
}

func truncateLogs(stepParams step.StepParams) step.StepExitError {
	// truncate systemd-journald's archived log files.  systemd may not
	// be in use and this is best-effort anyway: just log the outcome.
	cmd := []string{"journalctl", "--vacuum-size=1M"}
	_, err, _, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		log.Printf("Couldn't vacuum systemd journal: %v, stderr: %v", err, stderr)
	} else {
		log.Printf("Successfully vacuumed systemd journal")
	}

	logFilesToDelete, err := fileutils.GlobAll(deleteLogFilePatterns)
	if err != nil {
		errMsg := errors.Errorf("Unable to resolve file patterns '%v': %v", deleteLogFilePatterns, err)
		return step.ExitSafeToReboot{Err: errMsg}
	}

	for _, f := range logFilesToDelete {
		log.Printf("Removing '%v'", f)
		// ignore errors; this is best-effort only
		fileutils.RemoveFile(f)
	}

	logFilesToTruncate, err := fileutils.GlobAll(truncateLogFilePatterns)
	if err != nil {
		errMsg := errors.Errorf("Unable to resolve file patterns '%v': %v", deleteLogFilePatterns, err)
		return step.ExitSafeToReboot{Err: errMsg}
	}

	for _, f := range logFilesToTruncate {
		log.Printf("Truncating '%v'", f)
		// ignore errors; this is best-effort only
		fileutils.TruncateFile(f, 0)
	}
	return nil
}
