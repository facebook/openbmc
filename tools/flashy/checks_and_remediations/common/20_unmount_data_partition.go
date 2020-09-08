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
	"os"
	"strings"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/logger"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

const procMountsPath = "/proc/mounts"

func init() {
	step.RegisterStep(unmountDataPartition)
}

// unmountDataPartition unmounts /mnt/data, which is the data0 partition.
// A binding is made into tmpfs (/tmp/mnt/data), as some processes, such as sshd, rely on data
// inside /mnt/data.
// This step is required, as if the image layout is changed, the image's tail
// may be corrupted by the preexisting data0 partition.
// Also, should there be a need to format data0, this step is necessary.
func unmountDataPartition(stepParams step.StepParams) step.StepExitError {
	dataMounted, err := isDataPartitionMounted()
	if err != nil {
		return step.ExitSafeToReboot{
			errors.Errorf("Unable to determine whether /mnt/data is mounted: %v",
				err),
		}
	}

	if dataMounted {
		log.Printf("Found /mnt/data mounted, unmounting now.")
		err = runDataPartitionUnmountProcess()
		if err != nil {
			return step.ExitSafeToReboot{
				errors.Errorf("Failed to unmount /mnt/data: %v",
					err),
			}
		}
	} else {
		log.Printf("Skipping this step: /mnt/data not mounted.")
	}

	return nil
}

var isDataPartitionMounted = func() (bool, error) {
	procMountsDat, err := fileutils.ReadFile(procMountsPath)
	if err != nil {
		return false, errors.Errorf("Cannot read /proc/mounts: %v", err)
	}

	regEx := `(?m)^[^ ]+ /mnt/data [^ ]+ [^ ]+ [0-9]+ [0-9]+$`
	regExMap, err := utils.GetAllRegexSubexpMap(regEx, string(procMountsDat))
	if err != nil {
		return false, errors.Errorf("regex error: %v", err)
	}

	return len(regExMap) != 0, nil
}

var runDataPartitionUnmountProcess = func() error {
	// rsyslog will be killed by fuser, direct logs to stderr only.
	// Try to restart syslog at the end of this function, but if this fails
	// it is fine, as syslog will be restarted in the start of the next
	// step (as the CustomLogger gets the syslog writer).
	log.Printf("fuser will kill rsyslog and cause errors, turning off syslog logging now")
	log.SetOutput(os.Stderr)
	defer func() {
		logger.StartSyslog()
	}()

	// mkdir -p /tmp/mnt
	cmd := []string{"mkdir", "-p", "/tmp/mnt"}
	_, err, _, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	// mount --bind /mnt /tmp/mnt
	log.Printf("Bind mount /mnt to /tmp/mnt")
	cmd = []string{"mount", "--bind", "/mnt", "/tmp/mnt"}
	_, err, _, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	// cp -r /mnt/data /tmp/mnt
	log.Printf("Copying /mnt/data contents to /tmp/mnt.")
	cmd = []string{"cp", "-r", "/mnt/data", "/tmp/mnt"}
	_, err, _, stderr = utils.RunCommand(cmd, 2*time.Minute)
	if err != nil {
		return errors.Errorf("'%v', failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	log.Printf("Sleeping for 3s so that sshd closes any file descriptions in /mnt/data")
	utils.Sleep(3 * time.Second)

	// fuser -km /mnt/data
	log.Printf("Killing all processes accessing /mnt/data.")
	cmd = []string{"fuser", "-km", "/mnt/data"}
	_, err, _, stderr = utils.RunCommand(cmd, 30*time.Second)
	// we ignore the error in fuser, as this might be thrown because nothing holds
	// a file open on the file system (because of delayed log file open or because
	// rsyslogd is not running for some reason).
	// Retrying umount with a delay below should work around surviving processes.
	utils.LogAndIgnoreErr(err)

	// umount /mnt/data
	log.Printf("Unmounting /mnt/data.")
	cmd = []string{"umount", "/mnt/data"}
	_, err, _, stderr = utils.RunCommandWithRetries(cmd, 30*time.Second, 3, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v', failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	log.Printf("Making sure sshd config (including host keys) is still valid.")
	cmd = []string{"bash", "-c", "sshd -T 1> /dev/null"} // silence stdout (too verbose)
	_, err, _, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v', failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}
	return nil
}
