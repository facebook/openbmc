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

package install

import (
	"log"
	"os"
	"path/filepath"

	"github.com/facebook/openbmc/tools/flashy/lib/fileutils"
	"github.com/facebook/openbmc/tools/flashy/lib/step"
	"github.com/facebook/openbmc/tools/flashy/lib/utils"

	// all packages containing steps must be included to successfully run install
	_ "github.com/facebook/openbmc/tools/flashy/checks_and_remediations/common"
	_ "github.com/facebook/openbmc/tools/flashy/checks_and_remediations/wedge100"
	_ "github.com/facebook/openbmc/tools/flashy/flash_procedure"
)

// flashy _MUST_ be installed here
// checkInstallPath will confirm and error out if not
const flashyInstallPath = "/opt/flashy/flashy"

func Install(stepParams step.StepParams) {
	checkIsOpenBMC(stepParams.Clowntown)
	checkInstallPath()
	cleanInstallPath()
	initSymlinks()
}

// check whether the system is an OpenBMC
// allow 'clowntown' override to work around malformed /etc/issue
func checkIsOpenBMC(clowntown bool) {
	if clowntown {
		log.Printf("===== WARNING: Clowntown mode: Bypassing OpenBMC device " +
			"requirement check for installation =====")
		log.Printf("===== THERE IS RISK OF BRICKING THIS DEVICE =====")
		return
	}

	// instructions to bypass, appended to errors
	bypassMsg := "If you are on an OpenBMC machine with a malformed /etc/issue, " +
		"please reboot the device and try again. " +
		"Supply the `--clowntown` flag to bypass this check."

	isOpenBMC, err := utils.IsOpenBMC()
	if err != nil {
		log.Fatalf("Unable to check whether this device is an OpenBMC: %v. %v",
			err, bypassMsg)
	}

	if !isOpenBMC {
		log.Fatalf("Installation can only occur in an OpenBMC device! %v",
			bypassMsg)
	}
}

// check whether flashy is installed in the right path
func checkInstallPath() {
	exPath := fileutils.GetExecutablePath()
	if exPath != flashyInstallPath {
		log.Fatalf(`Unable to install flashy. Flashy should be installed in '%v',
 currently installed in '%v' instead.`, flashyInstallPath, exPath)
	}
}

// make sure the install directory is clean
func cleanInstallPath() {
	exPath := fileutils.GetExecutablePath()
	exDir := filepath.Dir(exPath)

	d, err := os.Open(exDir)
	defer d.Close()
	if err != nil {
		log.Fatalf("Unable to open installation directory: '%v'", err)
	}
	dirnames, err := d.Readdirnames(-1)
	if err != nil {
		log.Fatalf("Unable to read installation directory: '%v'", err)
	}

	for _, name := range dirnames {
		// don't delete flashy itself!
		if name != "flashy" {
			err := os.RemoveAll(filepath.Join(exDir, name))
			if err != nil {
				log.Fatalf("Unable to clean installation directory, '%v'", err)
			}
		}
	}
}

// init all the busybox-style symlinks
func initSymlinks() {
	// error handler
	errHandler := func(symlinkPath string, err error) {
		if err != nil {
			log.Fatalf("Unable to link '%v': '%v'", symlinkPath, err)
		}
	}

	exPath := fileutils.GetExecutablePath()
	exDir := filepath.Dir(exPath)

	for _symlinkPath, _ := range step.StepMap {
		// absolute symlinkPath
		symlinkPath := filepath.Join(exDir, _symlinkPath)

		// make directory up to the parent of the current path
		// this does nothing if the directory already exists
		mkdirErr := os.MkdirAll(filepath.Dir(symlinkPath), 0755)
		errHandler(symlinkPath, mkdirErr)

		// create symlink
		symlinkErr := os.Symlink(exPath, symlinkPath)
		errHandler(symlinkPath, symlinkErr)
	}
}
