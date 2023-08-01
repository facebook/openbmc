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
	"fmt"
	"log"
	"math/rand"
	"regexp"
	"strings"
	"time"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"
	"github.com/pkg/errors"
)

func init() {
	step.RegisterStep(ensureFlashWritable)
}

// A check for verified boot systems where the second flash is unwritable.
//
// On vboot BMCs the first chip is intended to be read-only in hardware and
// contains the Golden Image.  The second chip is intended to be read-write. 
// Some possible & obvious reasons this could go wrong:
//
// - the chips were installed in the wrong slots during manufacture.
// - the chips were erroneously swapped during repair (this is a valid
//   repair flow for non-vboot BMCs, though).
// - two read-only chips were installed.
//
// If the second chip is read-only, it's not possible to flash the BMC here. 
// Rebooting the BMC could have unpredictable consequences.  Leave these
// BMCs in place for later detection and repair.
//
// NB: it's conceivable that a non-vboot BMCs could boot up without an mtd
// partition for the U-Boot environment, so skip the check on those.
func ensureFlashWritable(stepParams step.StepParams) step.StepExitError {
	if !fileutils.FileExists("/usr/local/bin/vboot-util") {
		log.Printf("Non-vboot platform, skipping flash writability check")
		return nil
	}

	// S356523 workaround:
	// grandteton v2023.25.2 introduced an error whereby mtd0 was set as spi0.1
	// this breaks checks and remediations as it thinks flash1 is read-only
	// let's put in a workaround so we can upgrade all the affected hosts
	buf, err := fileutils.ReadFile("/proc/mtd")
	if err != nil {
		return step.ExitBadFlashChip{Err: err}
	
	}
	mtdStr := string(buf)
	mtdRegex := regexp.MustCompile(`mtd0:\s+\d+\s+\d+\s+"spi0\.1"`)
	mtdMatch := mtdRegex.FindStringSubmatch(mtdStr)
	if len(mtdMatch) > 0 {
		log.Printf("Skipping ensure_flash_writable check for this device due to S356523")
		return nil 
	}


	// First up check that fw_printenv works okay and produces output.
	cmd := []string{"fw_printenv", "bootargs"}
	_, err, stdout, stderr := utils.RunCommand(cmd, 30*time.Second)
	if err != nil || !strings.Contains(stdout, "bootargs") {
		log.Printf("fw_printenv doesn't work: %v, stderr: %v", err, stderr)
		return nil
	}

	// A dummy u-Boot environment variable & value.
	key := "_flashy_test"
	value := fmt.Sprintf("%08x", rand.Int31())

	// Set the variable and on failure just ignore the error.
	cmd = []string{"fw_setenv", key, value}
	_, err, stdout, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		log.Printf("fw_setenv doesn't work: %v, stderr: %v", err, stderr)
		return nil
	}

	// Did the set take hold?  If not the environment is read-only.
	var errRet step.StepExitError = nil
	cmd = []string{"fw_printenv", key}
	_, err, stdout, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil || !strings.Contains(stdout, value) {
		errMsg := errors.Errorf(
			"U-Boot environment is read only: flash chips swapped?" +
			" Error code: %v, stderr: %v", err, stderr)
		errRet = step.ExitBadFlashChip{Err: errMsg}
		log.Printf("U-Boot environment is READ ONLY")
	} else {
		log.Printf("U-Boot environment confirmed writable")
	}

	// Immediately clear even on error, in case something wacky happened.
	// The environment will typically be overwritten when the image is
	// flashed but cleaning up anyway doesn't hurt.
	cmd = []string{"fw_setenv", key}
	_, err, stdout, stderr = utils.RunCommand(cmd, 30*time.Second)
	if err != nil {
		log.Printf("Couldn't fw_setenv: %v, stderr: %v", err, stderr)
	}

	return errRet
}
