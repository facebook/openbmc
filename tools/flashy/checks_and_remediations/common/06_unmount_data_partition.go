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
	"syscall"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/logger"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(unmountDataPartition)
}

var mount = syscall.Mount

// unmountDataPartition attempts to unmount /mnt/data, which is the data0 partition.
// if unmount fails, it falls back to remounting /mnt/data RO.
// This step is required, as if the image layout is changed, the image's tail
// may be corrupted by the preexisting data0 partition.
// Also, should there be a need to format data0, this step is necessary.
func unmountDataPartition(stepParams step.StepParams) step.StepExitError {
	// With LF OpenBMC at Meta, /mnt/data is currently an eMMC device.
	// It won't be flashed so there's no need to unmount.
	if utils.IsLFOpenBMC() {
		log.Printf("Skipping this on LF OpenBMC.")
		return nil
	}

	dataMounted, err := utils.IsDataPartitionMounted()
	if err != nil {
		return step.ExitSafeToReboot{
			Err: errors.Errorf("Unable to determine whether /mnt/data is mounted: %v",
				err),
		}
	}

	if dataMounted {
		// rsyslog will be killed by fuser, direct logs to stderr only.
		// Try to restart syslog at the end of this function, but if this fails
		// it is fine, as syslog will be restarted in the start of the next
		// step (as the CustomLogger gets the syslog writer).
		log.Printf("fuser will kill rsyslog and cause errors, turning off syslog logging now")
		log.SetOutput(os.Stderr)
		defer func() {
			logger.StartSyslog()
		}()

		// attempt to unmount first, if failed, remount RO.
		log.Printf("Found /mnt/data mounted, unmounting now.")
		err = runDataPartitionUnmountProcess()
		if err != nil {
			log.Printf("/mnt/data unmount failed: %v", err)
			log.Printf("Falling back to remounting /mnt/data RO.")
			err = remountRODataPartition()
			if err != nil {
				log.Printf("/mnt/data remount failed: %v", err)
				return step.ExitSafeToReboot{
					Err: errors.Errorf("Failed to unmount or remount /mnt/data"),
				}
			}
		}

		// make sure sshd config still is valid
		err = validateSshdConfig()
		if err != nil {
			return step.ExitUnsafeToReboot{
				Err: errors.Errorf("Validate sshd config failed: %v", err),
			}
		}
	} else {
		log.Printf("Skipping this step: /mnt/data not mounted.")
	}

	return nil
}

// runDataPartitionUnmountProcess attempts (up to 10 times) to unmount /mnt/data
var runDataPartitionUnmountProcess = func() error {
	// mkdir -p /tmp/mnt
	// expected failures: none
	cmd := []string{"mkdir", "-p", "/tmp/mnt"}
	_, err, _, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	// equivalent to `mount --bind /mnt /tmp/mnt`, using a syscall as
	// --bind option to mount may not be available
	log.Printf("Bind mount /mnt to /tmp/mnt")
	err = mount("/mnt", "/tmp/mnt", "", syscall.MS_BIND, "")
	if err != nil {
		return errors.Errorf("Bind mount /mnt to /tmp/mnt failed: %v", err)
	}

	// mkdir -p /tmp/mnt/data/etc
	// expected failures: none
	cmd = []string{"mkdir", "-p", "/tmp/mnt/data/etc"}
	_, err, _, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}

	// cp -r /mnt/data/etc/ssh /tmp/mnt/data/etc
	// expected failures: jffs2 may be corrupt and throw errors
	log.Printf("Copying /mnt/data/etc/ssh to /tmp/mnt/data/etc.")
	cmd = []string{"cp", "-r", "/mnt/data/etc/ssh", "/tmp/mnt/data/etc"}
	_, err, _, stderr = utils.RunCommand(cmd, 2*time.Minute)
	if err != nil {
		return errors.Errorf("Copying /mnt/data/etc/ssh to /tmp/mnt/data/etc failed: "+
			"%v, stderr: %v", err, stderr)
	}

	// cp -r /mnt/data/kv_store /tmp/mnt/data/kv_store
	// expected failures: No such file or directory
	// (this is allowable as not all platforms have kv_store)
	log.Printf("Copying /mnt/data/kv_store to /tmp/mnt/data/kv_store")
	cmd = []string{"cp", "-r", "/mnt/data/kv_store", "/tmp/mnt/data/kv_store"}
	_, err, _, stderr = utils.RunCommand(cmd, 2*time.Minute)
	if err != nil && !strings.Contains(stderr, "No such file or directory"){
		return errors.Errorf("Copying /mnt/data/kv_store to /tmp/mnt/data/kv_store failed: "+
			"%v, stderr: %v", err, stderr)
	}

	// unmount /mnt/data
	// expected failures: fuser -km might not work, race conditions
	const tries = 10
	var lastAttemptError error
	for i := 1; i <= tries; i++ {
		log.Printf("Sleeping for 3s so that sshd closes any file descriptions in /mnt/data")
		utils.Sleep(3 * time.Second)

		killDataPartitionProcesses()

		// umount /mnt/data
		log.Printf("Trying to unmount /mnt/data (try %d of %d)", i, tries)
		cmd = []string{"umount", "/mnt/data"}
		_, lastAttemptError, _, _ = utils.RunCommand(cmd, 30*time.Second)
		if lastAttemptError != nil {
			log.Printf("umount (try %d of %d) failed: %v",
				i, tries, lastAttemptError)
		} else {
			return nil
		}

	}
	return lastAttemptError
}

var validateSshdConfig = func() error {
	log.Printf("Making sure sshd config (including host keys) is still valid.")
	cmd := []string{"bash", "-c", "sshd -T 1> /dev/null"} // silence stdout (too verbose)
	_, err, _, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}
	return nil
}

var killDataPartitionProcesses = func() {
	// fuser -km /mnt/data
	log.Printf("Killing all processes accessing /mnt/data")
	cmd := []string{"fuser", "-km", "/mnt/data"}
	_, err, _, _ := utils.RunCommand(cmd, 30*time.Second)

	// we ignore the error in fuser, as this might be thrown because nothing holds
	// a file open on the file system (because of delayed log file open or because
	// rsyslogd is not running for some reason).
	utils.LogAndIgnoreErr(err)

	// give signalled processes a chance to run and close descriptors
	if err == nil {
		utils.Sleep(time.Second)
	}
}

// remountRODataPartition remounts the data partition. This is a fallback
// solution in case unmounting fails.
var remountRODataPartition = func() error {
	killDataPartitionProcesses()

	log.Printf("Remounting /mnt/data read only")
	cmd := []string{"mount", "-o", "remount,ro", "/mnt/data"}
	_, err, _, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		return errors.Errorf("'%v' failed: %v, stderr: %v",
			strings.Join(cmd, " "), err, stderr)
	}
	return nil
}
